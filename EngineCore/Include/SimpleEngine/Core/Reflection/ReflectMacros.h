#pragma once

#include "SimpleEngine/Core/Reflection/AnnotationBase.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Utility/Common.h"

#include <concepts>
#include <tuple>
#include <type_traits>


/** 필드 하나에 어노테이션(태그) 값들을 붙입니다. */
#define SE_ANNOTATE(field, ...) \
    static constexpr auto _ANNO_VALUES_##field = std::make_tuple(__VA_ARGS__); \
    static_assert(::se::traits::UniqueTuple<decltype(_ANNO_VALUES_##field)>, \
        "SE_ANNOTATE(" #field "): the same tag type is attached to this field more than once."); \
    static constexpr auto _ANNO_REFS_##field = ::se::detail::MakeRefs(&_ANNO_VALUES_##field); \
    consteval bool _anno_check_##field() const \
    { \
        using SelfType = std::remove_cvref_t<decltype(*this)>; \
        static_assert( \
            []<typename U>() consteval { return requires { &U::field; }; }.operator()<SelfType>(), \
            "SE_ANNOTATE: no such field - " #field); \
        return true; \
    }

/**
 * 등록 블록(SE_REFLECT_BEGIN/END) 안에서, 필드 하나를 FieldInfo로 만들어 등록합니다.
 * 같은 이름의 필드에 SE_ANNOTATE가 있었다면 그 어노테이션 뷰를 자동으로 붙입니다.
 */
#define SE_FIELD(field) \
    { \
        fields.Push(::se::FieldInfo{ \
            .name = #field, \
            .type = ::se::TypeId::Of<std::remove_cvref_t<decltype(T::field)>>(), \
            .offset = ::se::detail::FieldOffsetOf<T>(&T::field), \
            .annotations = []<typename U>() consteval -> ::se::ArrayView<const ::se::AnnotationRef> \
            { \
                if constexpr (requires { U::_ANNO_REFS_##field; }) \
                { \
                    return U::_ANNO_REFS_##field; \
                } \
                else \
                { \
                    return {}; \
                } \
            }.operator()<T>(), \
        }); \
        ::se::EnsureRegistered<std::remove_cvref_t<decltype(T::field)>>(); \
    }

/**
 * 타입의 리플렉션 등록 블록을 시작합니다.
 * SE_DECLARE_REFLECTION(type)이 헤더에 먼저 선언되어 있어야 합니다.
 */
#define SE_REFLECT_BEGIN(type, ...) \
    static_assert(!::se::detail::IsRegistrarUnspecialized<type>, \
        "SE_REFLECT_BEGIN(" #type "): SE_DECLARE_REFLECTION(" #type ") must be declared in a header first."); \
    namespace { [[maybe_unused]] const bool SE_CONCAT_NAME(_se_reg_kick_, __LINE__) = (::se::EnsureRegistered<type>(), true); } \
    void ::se::Registrar<type>::Fill(::se::TypeInfo& info) \
    { \
        using T = type; \
        info.size = sizeof(T); \
        info.alignment = alignof(T); \
        info.name = ::se::TypeNameOf<T>(); \
        __VA_OPT__( \
            static constexpr auto TYPE_TAG_VALUES = std::make_tuple(__VA_ARGS__); \
            static_assert(::se::traits::UniqueTuple<decltype(TYPE_TAG_VALUES)>, \
                "SE_REFLECT_BEGIN(" #type "): the same tag type is attached more than once."); \
            static constexpr auto TYPE_TAG_REFS = ::se::detail::MakeRefs(&TYPE_TAG_VALUES); \
            info.annotations = TYPE_TAG_REFS; \
        ) \
        ::se::Array<::se::BaseInfo>& bases = ::se::TypeRegistry::Get().EmplaceBaseStorage(::se::TypeId::Of<T>()); \
        ::se::Array<::se::FieldInfo>& fields = ::se::TypeRegistry::Get().EmplaceFieldStorage(::se::TypeId::Of<T>());

/** 등록 블록 안에서, 부모 타입 하나를 BaseInfo로 만들어 등록합니다. */
#define SE_BASE(base_type) \
    { \
        static_assert(std::derived_from<T, base_type>, \
            "SE_BASE(" #base_type "): the registered type does not derive from it."); \
        bases.Push(::se::BaseInfo{ \
            .type = ::se::TypeId::Of<base_type>(), \
            .offset = ::se::detail::BaseOffsetOf<T, base_type>(), \
        }); \
        ::se::EnsureRegistered<base_type>(); \
    }

/** 타입의 리플렉션 등록 블록을 마칩니다. */
#define SE_REFLECT_END() \
        info.shape = ::se::StructInfo{ .bases = bases, .fields = fields }; \
    }

/**
 * enum의 리플렉션 등록 블록을 시작합니다.
 * 이름 조회가 필요 없는 enum은 이 매크로 없이도 자동으로(빈 entries) 등록됩니다.
 */
#define SE_REFLECT_ENUM_BEGIN(type) \
    static_assert(!::se::detail::IsRegistrarUnspecialized<type>, \
        "SE_REFLECT_ENUM_BEGIN(" #type "): SE_DECLARE_REFLECTION(" #type ") must be declared in a header first."); \
    namespace { [[maybe_unused]] const bool SE_CONCAT_NAME(_se_reg_enum_kick_, __LINE__) = (::se::EnsureRegistered<type>(), true); } \
    void ::se::Registrar<type>::Fill(::se::TypeInfo& info) \
    { \
        using T = type; \
        using UnderlyingType = std::underlying_type_t<T>; \
        info.size = sizeof(T); \
        info.alignment = alignof(T); \
        info.name = ::se::TypeNameOf<T>(); \
        ::se::Array<::se::EnumEntry>& entries = ::se::TypeRegistry::Get().EmplaceEnumEntryStorage(::se::TypeId::Of<T>()); \
        ::se::EnsureRegistered<UnderlyingType>();

/** enum 값 하나를 이름과 함께 등록합니다. */
#define SE_ENUM_VALUE(enumerate_name) \
    entries.Push(::se::EnumEntry{ \
        .value = static_cast<i64>(T::enumerate_name), \
        .name = #enumerate_name, \
    });

/** enum의 리플렉션 등록 블록을 마칩니다. */
#define SE_REFLECT_ENUM_END() \
        info.shape = ::se::EnumInfo{ \
            .underlying = ::se::TypeId::Of<UnderlyingType>(), \
            .entries = entries, \
        }; \
    }
