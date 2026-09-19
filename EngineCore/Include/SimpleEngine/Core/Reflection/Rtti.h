#pragma once

#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeRecord.h"
#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"
#include "SimpleEngine/Utility/Debug.h"

#include <algorithm>
#include <concepts>
#include <type_traits>


namespace se
{
/**
 * 런타임에 동적 타입 정보를 조회할 수 있는 타입입니다.
 * 루트 클래스에 SE_RTTI_ROOT(), 파생 클래스에 SE_RTTI(T)를 선언하여 만족시킵니다.
 */
template <typename T>
concept RuntimeTyped = requires (const T& object)
{
    { object.GetTypeRecord() } -> std::same_as<const TypeRecord*>;
};

/**
 * T의 캐스트 테이블 주소를 가져옵니다.
 * @note 최초 1회만 조회하고 이후에는 캐시된 포인터를 돌려줍니다.
 */
template <typename T>
[[nodiscard]] const TypeRecord* TypeRecordOf()
{
    static const TypeRecord* record = []
    {
        EnsureRegistered<T>();
        return &TypeRecordRegistry::Get().Find(TypeId::Of<T>()).Value();
    }();
    return record;
}

namespace detail
{
/** 캐스트 한 번에 필요한 두 오프셋과, 각각이 몇 번 나왔는지 */
struct CastLookup
{
    usize from_offset = 0;
    usize to_offset = 0;
    usize from_count = 0;
    usize to_count = 0;
};

/** all_bases를 한 번만 훑으며 from/to 오프셋을 동시에 수집합니다. */
[[nodiscard]] inline CastLookup LookupCast(const TypeRecord& record, TypeId from, TypeId to)
{
    CastLookup lookup;
    for (const CastEntry& entry : record.all_bases)
    {
        if (entry.type == from)
        {
            lookup.from_offset = entry.offset;
            ++lookup.from_count;
        }
        if (entry.type == to)
        {
            lookup.to_offset = entry.offset;
            ++lookup.to_count;
        }
    }
    return lookup;
}
} // namespace detail

/**
 * 포인터를 To로 캐스팅합니다. 불가능하거나 모호하면 nullptr입니다.
 * @note from이 두 번 이상 나오는 다이아몬드 구조는 완전 객체를 특정할 수 없어 실패시킵니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] To* Cast(From* instance)
{
    if (instance == nullptr)
    {
        return nullptr;
    }

    const TypeRecord* record = instance->GetTypeRecord();
    const detail::CastLookup lookup = detail::LookupCast(*record, TypeId::Of<From>(), TypeId::Of<To>());

    // from이 0개면 리플렉션 등록 코드에서 SE_BASE를 빠뜨린 것
    SE_ASSERT(lookup.from_count != 0, "Cast: the static type is missing from the dynamic type's base list. Did you forget SE_BASE?");

    if (lookup.from_count != 1 || lookup.to_count != 1)
    {
        return nullptr;
    }

    u8* const complete = reinterpret_cast<u8*>(instance) - lookup.from_offset;
    return reinterpret_cast<To*>(complete + lookup.to_offset);
}

/** const 포인터용 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To* Cast(const From* instance)
{
    return Cast<To>(const_cast<From*>(instance));
}

/** 실패하면 assert하는 캐스팅입니다. */
template <typename To, RuntimeTyped From>
[[nodiscard]] To* CastChecked(From* instance)
{
    To* result = Cast<To>(instance);
    SE_ASSERT(result != nullptr, "CastChecked failed: cannot cast to '{}'.", TypeNameOf<std::remove_cv_t<To>>());
    return result;
}

/** const 포인터용 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To* CastChecked(const From* instance)
{
    return CastChecked<To>(const_cast<From*>(instance));
}

/**
 * 동적 타입이 정확히 To일 때만 캐스팅합니다. 파생 클래스는 매칭되지 않습니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] To* ExactCast(From* instance)
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

    // 동적 타입이 곧 To이므로 from 오프셋만 빼면 됨
    const detail::CastLookup lookup = detail::LookupCast(*record, TypeId::Of<From>(), TypeId::Of<To>());
    if (lookup.from_count != 1)
    {
        return nullptr;
    }

    return reinterpret_cast<To*>(reinterpret_cast<u8*>(instance) - lookup.from_offset);
}

/** const 포인터용 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] const To* ExactCast(const From* instance)
{
    return ExactCast<To>(const_cast<From*>(instance));
}

/**
 * 동적 타입이 To이거나 To를 상속하는지 확인합니다.
 * @note 존재 여부만 묻는 것이므로 다이아몬드로 중복되어 있어도 true입니다.
 */
template <typename To, RuntimeTyped From>
[[nodiscard]] bool IsA(const From* instance)
{
    if (instance == nullptr)
    {
        return false;
    }

    const TypeId target = TypeId::Of<To>();
    const TypeRecord* record = instance->GetTypeRecord();
    return std::ranges::any_of(record->all_bases, [target](const CastEntry& entry)
    {
        return entry.type == target;
    });
}

/** 참조용 오버로드 */
template <typename To, RuntimeTyped From>
[[nodiscard]] bool IsA(const From& instance)
{
    return IsA<To>(&instance);
}
} // namespace se


/** 다형성 계층의 최상위 기본 클래스에 선언합니다. 파생 클래스는 SE_RTTI로 이를 구현해야 합니다. */
#define SE_RTTI_ROOT() \
    virtual const ::se::TypeRecord* GetTypeRecord() const = 0;

/** 구체 클래스에 선언합니다. 리플렉션 등록시, SE_BASE로 기본(부모) 클래스가 지정되어 있어야 합니다. */
#define SE_RTTI(type) \
    const ::se::TypeRecord* GetTypeRecord() const override { return ::se::TypeRecordOf<type>(); }
