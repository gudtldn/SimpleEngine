#pragma once

#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeRecord.h"
#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <algorithm>
#include <atomic>
#include <concepts>
#include <type_traits>


namespace se
{
/**
 * 런타임에 동적 타입 정보를 조회할 수 있는 타입인지 확인합니다.
 * @details 루트 클래스에 SE_RTTI_ROOT(), 파생 클래스에 SE_RTTI(T)를 선언해야 합니다.
 */
template <typename T>
concept RuntimeTyped = requires(const T& object)
{
    { object.GetTypeRecord() } -> std::same_as<const TypeRecord*>;
};

/**
 * T의 TypeRecord 주소를 반환합니다.
 * @note 최초 1회만 레지스트리에서 조회하고 이후에는 캐시된 포인터를 사용합니다.
 */
template <typename T>
[[nodiscard]] const TypeRecord* TypeRecordOf() noexcept
{
    // constinit을 사용하여 Magic Statics로 인한 데드락 방지
    static constinit std::atomic<const TypeRecord*> cached{ nullptr };
    if (const TypeRecord* record = cached.load(std::memory_order_acquire))
    {
        return record;
    }

    EnsureRegistered<T>();
    const TypeRecord* record = &TypeRecordRegistry::Get().Find(TypeId::Of<T>()).Value();
    cached.store(record, std::memory_order_release);
    return record;
}

/**
 * from_id 서브오브젝트 주소를 to_id 서브오브젝트 주소로 바꿉니다.
 * 정적 타입을 모르고 TypeId만 있을 때 씁니다.
 * @todo 나중에 inline과 성능 비교
 * @return 변환된 주소. 상속 관계가 없거나 모호한 경우 nullptr
 */
[[nodiscard]] SE_CORE_API void* CastById(void* instance, TypeId from_id, TypeId to_id, const TypeRecord& dynamic_record) noexcept;

/** const 포인터용 CastById 오버로드 */
[[nodiscard]] inline const void* CastById(const void* instance, TypeId from_id, TypeId to_id, const TypeRecord& dynamic_record) noexcept
{
    return CastById(const_cast<void*>(instance), from_id, to_id, dynamic_record);
}

/**
 * from_id 서브오브젝트 주소로부터 최하위 완전 객체(Most Derived Object) 주소를 구합니다.
 * 정적 타입을 모르고 TypeId만 있을 때 씁니다.
 * @return from_id가 dynamic_record에 없거나 두 번 이상 나오면 nullptr
 */
[[nodiscard]] SE_CORE_API void* CompleteObjectOfById(void* instance, TypeId from_id, const TypeRecord& dynamic_record) noexcept;

/** const 포인터용 CompleteObjectOfById 오버로드 */
[[nodiscard]] inline const void* CompleteObjectOfById(const void* instance, TypeId from_id, const TypeRecord& dynamic_record) noexcept
{
    return CompleteObjectOfById(const_cast<void*>(instance), from_id, dynamic_record);
}

/**
 * dynamic_id 타입이 target_id이거나 target_id를 상속하는지 확인합니다.
 * 정적 타입을 모를 때 씁니다.
 * @note dynamic_id와 target_id가 같으면 등록 여부와 관계없이 true, 그 밖에 등록되지 않은 dynamic_id는 false입니다.
 */
[[nodiscard]] SE_CORE_API bool IsAById(TypeId dynamic_id, TypeId target_id) noexcept;

/**
 * 포인터를 To 타입으로 캐스팅합니다. 불가능하거나 모호하면 nullptr를 반환합니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] To* Cast(From* instance) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }

    void* result = CastById(instance, TypeId::Of<From>(), TypeId::Of<To>(), *instance->GetTypeRecord());
    return static_cast<To*>(result);
}

/** const 포인터용 Cast 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To* Cast(const From* instance) noexcept
{
    return Cast<To>(const_cast<From*>(instance));
}

/**
 * 참조를 To 타입으로 캐스팅합니다.
 * @note 캐스팅 실패 시 assertion이 발생합니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] To& Cast(From& instance)
{
    To* result = Cast<To>(&instance);
    SE_ASSERT(result != nullptr, "Cast(Ref) failed: cannot cast reference to '{}'.", TypeNameOf<std::remove_cv_t<To>>());
    return *result;
}

/** const 참조용 Cast 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To& Cast(const From& instance)
{
    const To* result = Cast<To>(&instance);
    SE_ASSERT(result != nullptr, "Cast(Const Ref) failed: cannot cast reference to '{}'.", TypeNameOf<std::remove_cv_t<To>>());
    return *result;
}

/**
 * 포인터를 To 타입으로 캐스팅하며, 실패 시 assertion을 발생시킵니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] To* CastChecked(From* instance)
{
    To* result = Cast<To>(instance);
    SE_ASSERT(result != nullptr, "CastChecked failed: cannot cast to '{}'.", TypeNameOf<std::remove_cv_t<To>>());
    return result;
}

/** const 포인터용 CastChecked 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To* CastChecked(const From* instance)
{
    return CastChecked<To>(const_cast<From*>(instance));
}

/**
 * 동적 타입이 정확히 To와 일치할 때만 캐스팅합니다. (파생 타입 제외)
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] To* ExactCast(From* instance) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }

    const TypeRecord* record = instance->GetTypeRecord();
    if (record->id != TypeId::Of<To>())
    {
        return nullptr;
    }

    return static_cast<To*>(CompleteObjectOfById(instance, TypeId::Of<From>(), *record));
}

/** const 포인터용 ExactCast 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To* ExactCast(const From* instance) noexcept
{
    return ExactCast<To>(const_cast<From*>(instance));
}

/**
 * 서브오브젝트 주소로부터 최하위 완전 객체 주소를 구합니다. 동적 타입은 instance에서 조회합니다.
 */
template <RuntimeTyped Base>
[[nodiscard]] traits::CopyConst<Base, void*> CompleteObjectOf(Base* instance) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }
    return CompleteObjectOfById(
        const_cast<std::remove_cv_t<Base>*>(instance),
        TypeId::Of<std::remove_cv_t<Base>>(),
        *instance->GetTypeRecord()
    );
}

/**
 * 서브오브젝트 주소로부터 최하위 완전 객체 주소를 구합니다. 동적 타입의 TypeRecord를 직접 받습니다.
 * @return Base가 dynamic_record에 없거나 두 번 이상 나오면 nullptr
 */
template <typename Base>
    requires (!std::is_void_v<std::remove_cv_t<Base>>)
[[nodiscard]] traits::CopyConst<Base, void*> CompleteObjectOf(Base* instance, const TypeRecord& dynamic_record) noexcept
{
    return CompleteObjectOfById(
        const_cast<std::remove_cv_t<Base>*>(instance),
        TypeId::Of<std::remove_cv_t<Base>>(),
        dynamic_record
    );
}

/**
 * 인스턴스의 동적 타입이 To이거나 To를 상속하는지 확인합니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] bool IsA(const From* instance) noexcept
{
    if (instance == nullptr)
    {
        return false;
    }

    const TypeRecord* record = instance->GetTypeRecord();
    const TypeId target = TypeId::Of<To>();

    return std::ranges::any_of(record->all_bases, [target](const CastEntry& entry)
    {
        return entry.type == target;
    });
}

/** 참조형 인스턴스용 IsA 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] bool IsA(const From& instance) noexcept
{
    return IsA<To>(&instance);
}

/**
 * id 타입이 To이거나 To를 상속하는지 확인합니다.
 * @note id가 To와 같으면 등록 여부와 관계없이 true, 그 밖에 등록되지 않은 id는 false입니다.
 */
template <typename To>
[[nodiscard]] bool IsA(TypeId id) noexcept
{
    return IsAById(id, TypeId::Of<To>());
}
} // namespace se


/** 다형성 계층의 최상위 기본 클래스에 선언합니다. */
#define SE_RTTI_ROOT() \
    virtual const ::se::TypeRecord* GetTypeRecord() const = 0;

/** 구체 파생 클래스에 선언합니다. */
#define SE_RTTI(type) \
    const ::se::TypeRecord* GetTypeRecord() const override { return ::se::TypeRecordOf<type>(); }
