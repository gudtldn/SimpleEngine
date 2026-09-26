#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Core/Reflection/ValueOps.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/Utility/Debug.h"

#include <atomic>
#include <type_traits>
#include <variant>


namespace se
{
struct SerializePlan;

/** 평탄화된 구조체 필드 하나의 컴파일 결과 */
struct FieldStep
{
    StringView name;
    usize offset = 0;
    const SerializePlan* plan = nullptr;
};

/** 트레이트 또는 허용된 산술 타입의 leaf 노드 */
struct LeafStep
{
    SerializeOps ops{};
};

/** 베이스까지 평탄화된 필드 목록(선언 순서, 부모 필드가 먼저). */
struct StructSteps
{
    ArrayView<const FieldStep> fields;
};

/** Array-like 컨테이너의 원소 Plan과 ArrayOps */
struct ArraySteps
{
    const SerializePlan* element = nullptr;
    const ArrayOps* ops = nullptr;
};

/** Set/Map을 읽을 때 임시 원소를 만들기 위한 정보 */
struct ElementInfo
{
    usize size = 0;
    usize alignment = 0;
    const ValueOps* value_ops = nullptr;
};

/** Set-like 컨테이너의 원소 Plan과 SetOps */
struct SetSteps
{
    const SerializePlan* element = nullptr;
    const SetOps* ops = nullptr;
    ElementInfo element_info;
};

/** Map-like 컨테이너의 key/value Plan과 MapOps */
struct MapSteps
{
    const SerializePlan* key = nullptr;
    const SerializePlan* value = nullptr;
    const MapOps* ops = nullptr;
    ElementInfo key_info;
    ElementInfo value_info;
};

/** Optional의 내부 값 Plan과 OptionalOps */
struct OptionalSteps
{
    const SerializePlan* inner = nullptr;
    const OptionalOps* ops = nullptr;
};

/** enum을 쓸 정수 폭과 부호, 이름 목록 */
struct EnumStep
{
    EIntWidth width = EIntWidth::Bits32;
    bool is_signed = false;
    ArrayView<const EnumEntry> entries;
};

/** 형태별 직렬화 단계 */
using PlanSteps = std::variant<LeafStep, StructSteps, ArraySteps, SetSteps, MapSteps, OptionalSteps, EnumStep>;

/**
 * 타입 하나를 직렬화하는 데 필요한 정보를 리플렉션 TypeInfo에서 뽑아 둔 결과
 * 타입마다 한 번만 컴파일되고, 이후 직렬화할 때는 레지스트리 조회나 어노테이션 스캔 없이 바로 사용합니다.
 * @note 단일 스레드를 전제로 설계되었습니다. TryOf/Of는 한 스레드에서만 호출하고, 정적 초기화 중에는 호출하지 않습니다.
 */
struct SE_CORE_API SerializePlan
{
    TypeId type;
    PlanSteps steps;

    /**
     * 데이터에서 온 TypeId로 Plan을 컴파일하거나 캐시에서 가져옵니다.
     * 실패하면 이번 호출에서 넣은 슬롯을 모두 제거하고 오류를 반환합니다. 이전에 성공한 Plan은 그대로 둡니다.
     */
    [[nodiscard]] static Expected<const SerializePlan*, String> TryOf(TypeId id);

    /**
     * 정적 타입 T로 Plan을 가져옵니다. EnsureRegistered<T>()를 먼저 호출합니다.
     * @warning 컴파일에 실패하면 SE_ASSERT_RELEASE로 멈춥니다.
     */
    template <typename T>
    [[nodiscard]] static const SerializePlan& Of()
    {
        using CleanType = std::remove_cvref_t<T>;

        static constinit std::atomic<const SerializePlan*> cached{ nullptr };
        if (const SerializePlan* const plan = cached.load(std::memory_order_acquire))
        {
            return *plan;
        }

        EnsureRegistered<CleanType>();
        const auto result = TryOf(TypeId::Of<CleanType>());
        SE_ASSERT_RELEASE(result.HasValue(), "SerializePlan::Of<{}>: {}", TypeNameOf<CleanType>(), result.Error());

        cached.store(result.Value(), std::memory_order_release);
        return *result.Value();
    }
};
} // namespace se
