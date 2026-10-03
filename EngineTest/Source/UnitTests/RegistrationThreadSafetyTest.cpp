#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"

#include <atomic>
#include <latch>
#include <thread>
#include <type_traits>
#include <utility>

using namespace se;

// 여러 스레드가 처음 쓰는 타입을 동시에 등록하거나 Plan을 동시에 처음 컴파일해도 모두 같은 결과를 받는지 검증
namespace se_registration_thread_test
{
/** 등록 블록이 없어 처음 EnsureRegistered할 때 등록되는 enum입니다. 동시 등록 테스트에서만 씁니다. */
enum class ELazyValue : u16
{
    A,
    B,
};

/** 스냅샷 테스트에서 다른 스레드가 새로 등록하는 FixedArray의 원소 타입입니다. */
enum class ESnapshotValue : u8
{
    A,
};

/** 등록은 정적 초기화에서 끝나고, Plan은 동시 직렬화 테스트에서 처음 컴파일되는 재귀 타입입니다. */
struct PlanRaceNode
{
    i32 value = 0;
    String label;
    Array<PlanRaceNode> children;

    [[nodiscard]] bool operator==(const PlanRaceNode&) const = default;
};
} // namespace se_registration_thread_test

SE_DECLARE_REFLECTION(se_registration_thread_test::PlanRaceNode)

SE_REFLECT_BEGIN(se_registration_thread_test::PlanRaceNode)
    SE_FIELD(value)
    SE_FIELD(label)
    SE_FIELD(children)
SE_REFLECT_END()


namespace
{
constexpr usize THREAD_COUNT = 8;
constexpr usize SNAPSHOT_TYPE_COUNT = 16;

/** func를 THREAD_COUNT개의 스레드에서 한꺼번에 시작하고, 결과를 스레드 순서대로 돌려줍니다. */
template <typename Fn>
[[nodiscard]] Array<std::invoke_result_t<Fn&>> RunConcurrently(Fn func)
{
    Array<std::invoke_result_t<Fn&>> results;
    results.Resize(THREAD_COUNT);

    std::latch start{ THREAD_COUNT };
    {
        Array<std::jthread> threads;
        threads.Reserve(THREAD_COUNT);
        for (auto& result : results)
        {
            threads.Emplace([&start, &func, &result]
            {
                // 모든 스레드가 준비된 뒤 한꺼번에 시작해 첫 호출이 겹치게 함
                start.arrive_and_wait();
                result = func();
            });
        }
    } // jthread 소멸자에서 join

    return results;
}

/** 동시 직렬화 테스트에서 쓰는 PlanRaceNode 값을 만듭니다. */
[[nodiscard]] se_registration_thread_test::PlanRaceNode MakePlanRaceNode()
{
    using se_registration_thread_test::PlanRaceNode;
    return {
        .value = 1,
        .label = "root",
        .children = {
            PlanRaceNode{ .value = 2, .label = "leaf", .children = {} },
            PlanRaceNode{ .value = 3, .label = "branch", .children = { PlanRaceNode{ .value = 4, .label = "deep", .children = {} } } },
        },
    };
}

/** FixedArray<ESnapshotValue, 1..N>를 차례로 등록하고 각 TypeInfo를 돌려줍니다. */
template <usize... Indices>
[[nodiscard]] Array<const TypeInfo*> RegisterSnapshotTypes(std::index_sequence<Indices...>)
{
    return { &EnsureRegistered<FixedArray<se_registration_thread_test::ESnapshotValue, Indices + 1>>()... };
}
} // namespace


TEST(RegistrationThreadSafetyTest, ConcurrentEnsureRegisteredReturnsSameCompleteTypeInfo)
{
    using LazyContainer = HashMap<u32, Array<se_registration_thread_test::ELazyValue>>;
    ASSERT_FALSE(TypeRegistry::Get().Find(TypeId::Of<LazyContainer>()).HasValue());

    const Array<const TypeInfo*> results = RunConcurrently([]() -> const TypeInfo*
    {
        const TypeInfo& info = EnsureRegistered<LazyContainer>();

        // 돌려받은 즉시 등록의 마지막 단계(ValueOps, TypeRecord)까지 보여야 함
        const bool is_complete = info.AsMap().HasValue()
            && ValueOpsRegistry::Get().Find(info.id).HasValue()
            && TypeRecordRegistry::Get().Find(info.id).HasValue();
        return is_complete ? &info : nullptr;
    });

    ASSERT_NE(results[0], nullptr);
    for (const TypeInfo* const info : results)
    {
        EXPECT_EQ(info, results[0]);
    }
}

TEST(RegistrationThreadSafetyTest, ConcurrentFirstPlanCompileReturnsSamePlanAndBytes)
{
    using se_registration_thread_test::PlanRaceNode;

    /** 스레드 하나가 받은 Plan과 그 Plan으로 쓴 바이트 */
    struct Output
    {
        const SerializePlan* plan = nullptr;
        Array<u8> bytes;
    };

    const Array<Output> outputs = RunConcurrently([]
    {
        const SerializePlan& plan = SerializePlanOf<PlanRaceNode>();
        const PlanRaceNode value = MakePlanRaceNode();

        Array<u8> bytes;
        BinaryWriter writer(bytes);
        const bool is_written = serde::Serialize(writer, plan, &value).HasValue();
        return Output{ .plan = &plan, .bytes = is_written ? bytes : Array<u8>{} };
    });

    ASSERT_FALSE(outputs[0].bytes.IsEmpty());
    for (const Output& output : outputs)
    {
        EXPECT_EQ(output.plan, outputs[0].plan);
        EXPECT_EQ(output.bytes, outputs[0].bytes);
    }

    // 모든 스레드가 쓴 바이트가 원래 값으로 돌아오는지 확인
    BinaryReader reader(outputs[0].bytes);
    PlanRaceNode result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());
    EXPECT_EQ(result, MakePlanRaceNode());
}

TEST(RegistrationThreadSafetyTest, GetAllTypesSnapshotWhileAnotherThreadRegisters)
{
    using se_registration_thread_test::ESnapshotValue;
    ASSERT_FALSE(TypeRegistry::Get().Find(TypeId::Of<FixedArray<ESnapshotValue, 1>>()).HasValue());

    std::atomic<bool> is_done{ false };
    std::latch start{ 2 };
    Array<const TypeInfo*> registered;
    std::jthread registering{ [&]
    {
        start.arrive_and_wait();
        registered = RegisterSnapshotTypes(std::make_index_sequence<SNAPSHOT_TYPE_COUNT>{});
        is_done.store(true, std::memory_order_release);
    } };

    // 다른 스레드가 등록하는 동안 스냅샷을 계속 뜸. 레지스트리는 줄지 않으므로 길이도 줄지 않아야 함
    start.arrive_and_wait();
    usize previous_len = 0;
    do
    {
        const Array<const TypeInfo*> snapshot = TypeRegistry::Get().GetAllTypes();
        EXPECT_GE(snapshot.Len(), previous_len);
        previous_len = snapshot.Len();
    }
    while (!is_done.load(std::memory_order_acquire));
    registering.join();

    // 스냅샷의 포인터는 레지스트리가 가진 슬롯이고, 새로 등록한 타입이 모두 담겨야 함
    const Array<const TypeInfo*> final_snapshot = TypeRegistry::Get().GetAllTypes();
    for (const TypeInfo* const info : final_snapshot)
    {
        const auto found = TypeRegistry::Get().Find(info->id);
        ASSERT_TRUE(found.HasValue());
        EXPECT_EQ(&found.Value(), info);
    }
    for (const TypeInfo* const info : registered)
    {
        EXPECT_TRUE(final_snapshot.Contains(info));
    }
}
