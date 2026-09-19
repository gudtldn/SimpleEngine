// ReSharper disable CppUnusedIncludeDirective
#pragma once

#include "SimpleEngine/Core/Serialization/Legacy/MathSerialize.h"
#include "Annotations.h"
#include "Enum.h"
#include "SimpleEngine/Core/Reflection/Legacy/Meta.h"
#include "SimpleEngine/Core/Reflection/Legacy/TagTraits.h"
#include "TypeRegistry.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Common.h"

#include <type_traits>
#include <concepts>


// TODO: C++26되면 여기 파일 전체 수정해야 함

namespace se::detail
{
template <typename T, auto... Tags>
    requires (std::derived_from<decltype(Tags), se::meta::target::Type> && ...)
consteval BitFlags<ETypeFlags_v1> MakeTypeFlags_v1()
{
    BitFlags<ETypeFlags_v1> flags;

    // Abstract 클래스 자동 감지
    if constexpr (std::is_abstract_v<T>)
    {
        flags |= ETypeFlags_v1::IsAbstract;
    }

    auto process_tag = [&flags]<auto Tag>
    {
        using TagType = std::remove_cvref_t<decltype(Tag)>;
        flags |= TypeFlagTrait_v1<TagType>::VALUE;
    };

    (process_tag.template operator()<Tags>(), ...);
    return flags;
}

template <auto... Tags>
    requires (std::derived_from<decltype(Tags), se::meta::target::Field> && ...)
consteval PropertyMetadata_v1 MakePropertyMetadata_v1()
{
    PropertyMetadata_v1 meta{};
    auto process_tag = [&meta]<auto Tag>
    {
        using TagType = std::remove_cvref_t<decltype(Tag)>;
        PropertyMetadataTrait_v1<TagType>::Apply(meta, Tag);
    };

    (process_tag.template operator()<Tags>(), ...);
    return meta;
}

template <typename T, auto... Tags>
    requires (std::derived_from<decltype(Tags), se::meta::target::Type> && ...)
void DispatchRegistrationHooks_v1()
{
    auto process_tag = []<auto Tag>
    {
        using TagType = std::remove_cvref_t<decltype(Tag)>;
        constexpr bool implemented = requires { RegistrationTrait_v1<TagType>::template Apply<T>(); };

        static_assert(!HookRequiredTrait_v1<TagType>::VALUE || implemented,
            "This tag requires a RegistrationTrait<TagType>::Apply<T>() specialization. "
            "Please check if the hook header for the consuming module is included. "
            "(Also fires if Apply<T> exists but is constrained and T fails the constraint - "
            "Apply must be unconstrained; enforce requirements via static_assert inside the body.)");

        if constexpr (implemented)
        {
            RegistrationTrait_v1<TagType>::template Apply<T>();
        }
    };
    (process_tag.template operator()<Tags>(), ...);
}
} // namespace se::detail

#define SE_INTERNAL_CLASS_BODY(this_class, base_class, override_keyword, static_assert_expr) \
private: \
    friend class ::se::detail::TypeBuilder_v1<this_class>; \
    friend struct this_class##_Registrar; \
    using Super = base_class; \
    using ThisClass = this_class; \
public: \
    static const ::se::TypeInfo_v1& StaticTypeInfo() \
    { \
        static_assert_expr \
        static const TypeInfo_v1& info = ::se::TypeRegistry_v1::Get().FindChecked<this_class>(); \
        return info; \
    } \
    virtual const ::se::TypeInfo_v1& GetTypeInfo() const override_keyword \
    { \
        return StaticTypeInfo(); \
    } \
    virtual ::se::TypeId_v1 GetTypeId() const override_keyword \
    { \
        return ::se::TypeId_v1::Of<this_class>(); \
    } \
    virtual void* GetCompleteObject() override_keyword \
    { \
        return this; \
    }

#define SE_INTERNAL_CLASS_DEFAULT(this_class) \
    SE_INTERNAL_CLASS_BODY(this_class, void,,)

#define SE_INTERNAL_CLASS_WITH_BASE(this_class, base_class) \
    SE_INTERNAL_CLASS_BODY(this_class, base_class, override, static_assert(std::derived_from<this_class, base_class>, #this_class " must inherit from " #base_class);)

#define SE_INTERNAL_GET_OVERLOADED_CLASS_MACRO(_1, _2, macro, ...) macro

/**
 * 클래스 정의 내부에 선언해야 하는 리플렉션 매크로입니다.
 * 인자 개수에 따라 부모 클래스 유무를 자동으로 판단하여 적절한 코드를 생성합니다.
 *
 * 사용법:
 * - 루트 클래스: SE_CLASS(MyClass)
 * - 파생 클래스: SE_CLASS(MyClass, MyBaseClass)
 */
#define SE_CLASS_V1(...) \
    SE_EXPAND_MACRO(SE_INTERNAL_GET_OVERLOADED_CLASS_MACRO(__VA_ARGS__, SE_INTERNAL_CLASS_WITH_BASE, SE_INTERNAL_CLASS_DEFAULT))(__VA_ARGS__)

/**
 * 타입의 리플렉션 정보 등록을 시작합니다.
 * @param type 등록할 클래스/구조체 이름
 * @param ... 클래스 속성 태그
 */
#define SE_BEGIN_REFLECT_V1(type, ...) \
[[maybe_unused]] inline static const bool SE_CONCAT_NAME(_Reflect_Init_, type) = [] static -> bool \
{ \
    using T = type; \
    constexpr auto type_flags = ::se::detail::MakeTypeFlags_v1<T __VA_OPT__(,) __VA_ARGS__>(); \
    /* 훅은 등록 완료 후(SE_END_REFLECT) 실행합니다 — END는 태그 팩을 받지 않으므로 여기서 캡처합니다. */ \
    const auto dispatch_hooks = [] { ::se::detail::DispatchRegistrationHooks_v1<T __VA_OPT__(,) __VA_ARGS__>(); }; \
    ::se::TypeRegistry_v1::Get().Register<T>() \
        .AddFlags(type_flags)

/**
 * 인터페이스(Interface)를 등록합니다.
 * @param ... 인터페이스 목록
 */
#define SE_REFLECT_INTERFACE_V1(...) \
        .Implements<__VA_ARGS__>()

// NOLINTBEGIN(bugprone-macro-parentheses)
/**
 * 멤버 변수(Property)를 등록합니다.
 * @param member 멤버 변수
 * @param ... 프로퍼티 속성 태그
 */
#define SE_REFLECT_PROPERTY_V1(member, ...) \
        .Property<&T::member>(SE_STRINGIFY(member)) \
        .ApplyMetadata(::se::detail::MakePropertyMetadata_v1<__VA_ARGS__>())
// NOLINTEND(bugprone-macro-parentheses)

/** 타입의 리플렉션 정보 등록을 마칩니다. */
#define SE_END_REFLECT_V1(type) \
    ; /* 체이닝 종료 */ \
    static_assert(std::same_as<std::decay_t<T>, std::decay_t<type>>, "Type mismatch between BEGIN and END reflect macros."); \
    dispatch_hooks(); /* 등록 완료 후 훅 실행 — 훅에서 TypeRegistry의 TypeInfo를 조회할 수 있습니다. */ \
    return true; \
}();

/**
 * 열거형(Enum)의 리플렉션 정보를 등록합니다.
 * underlying type의 TypeId가 자동으로 inner_type_id에 설정되며,
 * underlying type 기반 직렬화 콜백과 Enum 항목 목록 접근자가 자동 등록됩니다.
 *
 * @note 이 매크로를 사용하는 파일에서 Archive.h가 포함되어야 합니다.
 * @param enum_type 등록할 열거형 이름
 */
#define SE_REFLECT_ENUM_V1(enum_type) \
inline static const bool SE_CONCAT_NAME(_Reflect_Init_Enum_, enum_type) = [] static -> bool \
{ \
    using T = enum_type; \
    ::se::BitFlags<::se::ETypeFlags_v1> enum_flags; \
    if constexpr (::se::detail::EnumReflector_v1<T>::IsBitFlag) \
    { \
        enum_flags |= ::se::ETypeFlags_v1::IsBitFlag; \
    } \
    if constexpr (std::is_unsigned_v<std::underlying_type_t<T>>) \
    { \
        enum_flags |= ::se::ETypeFlags_v1::IsUnsigned; \
    } \
    ::se::TypeRegistry_v1::Get().RegisterEnum<T>() \
        .AddFlags(enum_flags) \
        .Serialize([](::se::Archive_v1& ar, void* p) static { ar << *static_cast<T*>(p); }) \
        .EnumEntries([](const ::se::EnumEntry_v1*& out_data, usize& out_count) static \
        { \
            constexpr auto& entries = ::se::detail::EnumReflector_v1<T>::Entries; \
            out_data = entries.Data(); \
            out_count = entries.Len(); \
        }); \
    return true; \
}();
