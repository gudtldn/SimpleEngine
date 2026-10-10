#pragma once

#include "SimpleEngine/Core/Reflection/AnnotationBase.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/RegistrationTraits.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"
#include "SimpleEngine/Core/Reflection/TypeName.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Traits/TupleTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Common.h"

#include <concepts>
#include <tuple>
#include <type_traits>


/** 필드 하나에 어노테이션 값들을 붙입니다. */
#define SE_ANNOTATE(field, ...) \
    static constexpr auto _ANNO_VALUES_##field = std::make_tuple(__VA_ARGS__); \
    static_assert(::se::traits::UniqueTuple<decltype(_ANNO_VALUES_##field)>, \
        "SE_ANNOTATE(" #field "): the same annotation type is attached to this field more than once."); \
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

/** 타입 어노테이션 튜플과, 등록 뒤 RegistrationTraits를 부르는 정적 초기화 변수를 정의합니다. */
#define SE_DETAIL_REFLECT_AUTOREG(macro_name, type, ...) \
    namespace \
    { \
        constexpr auto SE_LINE_NAME(_se_type_annos_) = std::make_tuple(__VA_ARGS__); \
        static_assert(::se::traits::UniqueTuple<decltype(SE_LINE_NAME(_se_type_annos_))>, \
            SE_STRINGIFY(macro_name) "(" #type "): the same annotation type is attached more than once."); \
        constexpr auto SE_LINE_NAME(_se_type_anno_refs_) = ::se::detail::MakeRefs(&SE_LINE_NAME(_se_type_annos_)); \
        [[maybe_unused]] const bool SE_LINE_NAME(_se_reg_kick_) = [] \
        { \
            ::se::EnsureRegistered<type>(); \
            ::se::detail::RunRegistrationTraits<type>(SE_LINE_NAME(_se_type_annos_)); \
            return true; \
        }(); \
    }

/**
 * 타입의 리플렉션 등록 블록을 시작합니다.
 * SE_DECLARE_REFLECTION(type)이 헤더에 먼저 선언되어 있어야 합니다.
 */
#define SE_REFLECT_BEGIN(type, ...) \
    static_assert(::se::traits::IsSpecialized<::se::Registrar, type>, \
        "SE_REFLECT_BEGIN(" #type "): SE_DECLARE_REFLECTION(" #type ") must be declared in a header first."); \
    SE_DETAIL_REFLECT_AUTOREG(SE_REFLECT_BEGIN, type __VA_OPT__(,) __VA_ARGS__) \
    void ::se::Registrar<type>::Fill(::se::TypeInfo& info) \
    { \
        using T = type; \
        info.size = sizeof(T); \
        info.alignment = alignof(T); \
        info.name = ::se::TypeNameOf<T>(); \
        info.annotations = ::se::AnnotationList{ SE_LINE_NAME(_se_type_anno_refs_) }; \
        auto& [bases, fields] = ::se::TypeRegistry::Get().EmplaceStructStorage(::se::TypeId::Of<T>());

/**
 * 등록 블록 안에서, 부모 타입 하나를 BaseInfo로 만들어 등록합니다.
 * @note 가상 상속은 최종 파생 타입에 따라 오프셋이 달라지므로 지원하지 않습니다.
 */
#define SE_BASE(base_type) \
    { \
        static_assert(std::derived_from<T, base_type>, \
            "SE_BASE(" #base_type "): the registered type does not derive from it."); \
        static_assert(::se::traits::StaticCastableTo<base_type*, T*>, \
            "SE_BASE(" #base_type "): virtual inheritance is not supported."); \
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
#define SE_REFLECT_ENUM_BEGIN(type, ...) \
    static_assert(::se::traits::IsSpecialized<::se::Registrar, type>, \
        "SE_REFLECT_ENUM_BEGIN(" #type "): SE_DECLARE_REFLECTION(" #type ") must be declared in a header first."); \
    SE_DETAIL_REFLECT_AUTOREG(SE_REFLECT_ENUM_BEGIN, type __VA_OPT__(,) __VA_ARGS__) \
    void ::se::Registrar<type>::Fill(::se::TypeInfo& info) \
    { \
        using T = type; \
        using UnderlyingType = std::underlying_type_t<T>; \
        info.size = sizeof(T); \
        info.alignment = alignof(T); \
        info.name = ::se::TypeNameOf<T>(); \
        info.annotations = ::se::AnnotationList{ SE_LINE_NAME(_se_type_anno_refs_) }; \
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
