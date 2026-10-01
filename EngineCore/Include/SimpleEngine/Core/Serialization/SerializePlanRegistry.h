#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Serialization/SerializePlan.h"
#include "SimpleEngine/Utility/Debug.h"

#include <atomic>
#include <type_traits>


namespace se
{
// forward declaration
class SerializePlanCompiler;

/**
 * 타입마다 컴파일한 SerializePlan을 소유하는 전역 레지스트리
 */
class SE_CORE_API SerializePlanRegistry
{
    SerializePlanRegistry() = default;

public:
    /** 전역 싱글톤 인스턴스를 가져옵니다. */
    [[nodiscard]] static SerializePlanRegistry& Get();

    /**
     * 데이터에서 온 TypeId로 Plan을 찾거나 컴파일합니다.
     * 실패하면 이번 호출에서 넣은 슬롯을 모두 제거하고 오류를 반환합니다. 이전에 성공한 Plan은 그대로 둡니다.
     */
    [[nodiscard]] Expected<const SerializePlan*, String> FindOrCompile(TypeId id);

private:
    friend class SerializePlanCompiler;

    /** Plan 하나와, 그 Plan의 StructSteps가 가리키는 FieldStep 배열 */
    struct Slot
    {
        SerializePlan plan;
        Array<FieldStep> fields;
    };

    /** 각 타입의 슬롯 */
    HashMap<TypeId, Slot> slots;
};

/**
 * 정적 타입 T의 Plan을 가져옵니다. EnsureRegistered<T>()를 먼저 호출합니다.
 * @warning 컴파일에 실패하면 SE_ASSERT_RELEASE로 멈춥니다.
 */
template <typename T>
[[nodiscard]] const SerializePlan& SerializePlanOf()
{
    using CleanType = std::remove_cvref_t<T>;

    static constinit std::atomic<const SerializePlan*> cached{ nullptr };
    if (const SerializePlan* const plan = cached.load(std::memory_order_acquire))
    {
        return *plan;
    }

    EnsureRegistered<CleanType>();
    const auto result = SerializePlanRegistry::Get().FindOrCompile(TypeId::Of<CleanType>());
    SE_ASSERT_RELEASE(result.HasValue(), "SerializePlanOf<{}>: {}", TypeNameOf<CleanType>(), result.Error());

    cached.store(result.Value(), std::memory_order_release);
    return *result.Value();
}
} // namespace se
