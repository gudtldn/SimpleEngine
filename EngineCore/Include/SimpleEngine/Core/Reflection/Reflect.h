// ReSharper disable CppUnusedIncludeDirective
#pragma once

#include "SimpleEngine/Core/Math/MathSerialize.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/Enum.h"
#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Common.h"

#include <type_traits>
#include <concepts>


// TODO: C++26되면 여기 파일 전체 수정해야 함

namespace se::detail
{
template <typename T, auto... Tags>
    requires (std::derived_from<decltype(Tags), se::meta::target::Type> && ...)
consteval BitFlags<ETypeFlags> MakeTypeFlags()
{
    using namespace se::meta;

    BitFlags<ETypeFlags> flags;

    // Abstract 클래스 자동 감지
    if constexpr (std::is_abstract_v<T>)
    {
        flags |= ETypeFlags::IsAbstract;
    }

    auto process_tag = [&flags]<auto Tag>()
    {
        using TagType = std::remove_cvref_t<decltype(Tag)>;

        // Reflect, ECS 관련 Tags
        if constexpr (
            std::same_as<TagType, tags::Reflect>
            || std::same_as<TagType, tags::Component>
            || std::same_as<TagType, tags::Resource>
        )
        {
            // DeduceECSKind에서 enum_kind 설정
            // 여기는 MakeTypeFlags에서 static_assert 방지용 trap
        }

        // 가시성/직렬화 수정자 (Type 레벨)
        else if constexpr (std::same_as<TagType, tags::Hidden>)
        {
            flags |= ETypeFlags::Hidden; // 에디터에서 숨김
        }
        else if constexpr (std::same_as<TagType, tags::Transient>)
        {
            flags |= ETypeFlags::Transient; // 저장 안 함
        }

        // Manual Override Tags
        else if constexpr (std::same_as<TagType, tags::Abstract>)
        {
            flags |= ETypeFlags::IsAbstract;
        }

        // Fallback
        else
        {
            static_assert(se::traits::AlwaysFalse<TagType>, "Invalid type tag");
        }
    };

    (process_tag.template operator()<Tags>(), ...);
    return flags;
}

template <auto... Tags>
    requires (std::derived_from<decltype(Tags), se::meta::target::Field> && ...)
consteval PropertyMetadata MakePropertyMetadata()
{
    using namespace se::meta;

    PropertyMetadata meta{};
    auto process_tag = [&meta]<auto Tag>()
    {
        using TagType = std::remove_cvref_t<decltype(Tag)>;

        // --- Marker Tags (Flags) ---
        if constexpr (std::same_as<TagType, tags::ReadOnly>)
        {
            meta.flags |= EPropertyFlags::ReadOnly;
        }
        else if constexpr (std::same_as<TagType, tags::Advanced>)
        {
            meta.flags |= EPropertyFlags::Advanced;
        }
        else if constexpr (std::same_as<TagType, tags::Hidden>)
        {
            meta.flags |= EPropertyFlags::Hidden;
        }
        else if constexpr (std::same_as<TagType, tags::Transient>)
        {
            meta.flags |= EPropertyFlags::Transient;
        }

        // --- Payload Tags (Data) ---
        else if constexpr (std::derived_from<TagType, tags::DisplayNameBase>)
        {
            meta.display_name = TagType::value;
        }
        else if constexpr (std::derived_from<TagType, tags::CategoryBase>)
        {
            meta.category = TagType::value;
        }
        else if constexpr (std::derived_from<TagType, tags::TooltipBase>)
        {
            meta.tooltip = TagType::value;
        }

        else if constexpr (std::derived_from<TagType, tags::RangeBase>)
        {
            meta.flags |= EPropertyFlags::HasRange;
            meta.range_min = static_cast<f32>(Tag.min);
            meta.range_max = static_cast<f32>(Tag.max);
        }

        // Fallback
        else
        {
            // se::meta::Reflect 태그는 단순 마커이므로 무시
            if constexpr (!std::same_as<TagType, tags::Reflect>)
            {
                static_assert(se::traits::AlwaysFalse<TagType>, "Unhandled property tag encountered.");
            }
        }
    };

    (process_tag.template operator()<Tags>(), ...);
    return meta;
}

template <auto... Tags>
    requires (std::derived_from<decltype(Tags), se::meta::target::Type> && ...)
consteval EECSKind DeduceECSKind()
{
    using namespace se::meta;

    constexpr usize ecs_tag_count =
        (0 + ... + static_cast<usize>(std::same_as<std::remove_cvref_t<decltype(Tags)>, tags::Component>))
      + (0 + ... + static_cast<usize>(std::same_as<std::remove_cvref_t<decltype(Tags)>, tags::Resource>));
    static_assert(ecs_tag_count <= 1, "A type cannot be both Component and Resource.");

    EECSKind kind = EECSKind::None;
    auto process = [&kind]<auto Tag>()
    {
        using TagType = std::remove_cvref_t<decltype(Tag)>;
        if constexpr (std::same_as<TagType, tags::Component>)
        {
            kind = EECSKind::Component;
        }
        else if constexpr (std::same_as<TagType, tags::Resource>)
        {
            kind = EECSKind::Resource;
        }
    };
    (process.template operator()<Tags>(), ...);
    return kind;
}
} // namespace se::detail

#define SE_INTERNAL_CLASS_BODY(this_class, base_class, override_keyword, static_assert_expr) \
private: \
    friend class ::se::detail::TypeBuilder<this_class>; \
    friend struct this_class##_Registrar; \
    using Super = base_class; \
    using ThisClass = this_class; \
public: \
    static const ::se::TypeInfo& StaticTypeInfo() \
    { \
        static_assert_expr \
        static const TypeInfo& info = ::se::TypeRegistry::Get().FindChecked<this_class>(); \
        return info; \
    } \
    virtual const ::se::TypeInfo& GetTypeInfo() const override_keyword \
    { \
        return StaticTypeInfo(); \
    } \
    virtual ::se::TypeId GetTypeId() const override_keyword \
    { \
        return ::se::TypeId::Of<this_class>(); \
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
#define SE_CLASS(...) \
    SE_EXPAND_MACRO(SE_INTERNAL_GET_OVERLOADED_CLASS_MACRO(__VA_ARGS__, SE_INTERNAL_CLASS_WITH_BASE, SE_INTERNAL_CLASS_DEFAULT))(__VA_ARGS__)

/**
 * 타입의 리플렉션 정보 등록을 시작합니다.
 * @param type 등록할 클래스/구조체 이름
 * @param ... 클래스 속성 태그
 */
#define SE_BEGIN_REFLECT(type, ...) \
[[maybe_unused]] inline static const bool SE_CONCAT_NAME(_Reflect_Init_, type) = [] static -> bool \
{ \
    using T = type; \
    constexpr auto type_flags = ::se::detail::MakeTypeFlags<T __VA_OPT__(,) __VA_ARGS__>(); \
    constexpr auto ecs_kind = ::se::detail::DeduceECSKind<__VA_ARGS__>(); \
    ::se::TypeRegistry::Get().Register<T>() \
        .AddFlags(type_flags) \
        .SetECSKind(ecs_kind)

/**
 * 인터페이스(Interface)를 등록합니다.
 * @param ... 인터페이스 목록
 */
#define SE_REFLECT_INTERFACE(...) \
        .Implements<__VA_ARGS__>()

// NOLINTBEGIN(bugprone-macro-parentheses)
/**
 * 멤버 변수(Property)를 등록합니다.
 * @param member 멤버 변수
 * @param ... 프로퍼티 속성 태그
 */
#define SE_REFLECT_PROPERTY(member, ...) \
        .Property<&T::member>(SE_STRINGIFY(member)) \
        .ApplyMetadata(::se::detail::MakePropertyMetadata<__VA_ARGS__>())
// NOLINTEND(bugprone-macro-parentheses)

/** 타입의 리플렉션 정보 등록을 마칩니다. */
#define SE_END_REFLECT(type) \
    ; /* 체이닝 종료 */ \
    static_assert(std::same_as<std::decay_t<T>, std::decay_t<type>>, "Type mismatch between BEGIN and END reflect macros."); \
    if constexpr (ecs_kind == ::se::EECSKind::Component) \
    { \
        ::se::ECSRegistry::Get().RegisterComponentOps<type>(); \
    } \
    else if constexpr (ecs_kind == ::se::EECSKind::Resource) \
    { \
        ::se::ECSRegistry::Get().RegisterResourceOps<type>(); \
    } \
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
#define SE_REFLECT_ENUM(enum_type) \
inline static const bool SE_CONCAT_NAME(_Reflect_Init_Enum_, enum_type) = [] static -> bool \
{ \
    using T = enum_type; \
    ::se::BitFlags<::se::ETypeFlags> enum_flags; \
    if constexpr (::se::detail::EnumReflector<T>::IsBitFlag) \
    { \
        enum_flags |= ::se::ETypeFlags::IsBitFlag; \
    } \
    if constexpr (std::is_unsigned_v<std::underlying_type_t<T>>) \
    { \
        enum_flags |= ::se::ETypeFlags::IsUnsigned; \
    } \
    ::se::TypeRegistry::Get().RegisterEnum<T>() \
        .AddFlags(enum_flags) \
        .Serialize([](::se::Archive& ar, void* p) static { ar << *static_cast<T*>(p); }) \
        .EnumEntries([](const ::se::EnumEntry*& out_data, usize& out_count) static \
        { \
            constexpr auto& entries = ::se::detail::EnumReflector<T>::Entries; \
            out_data = entries.Data(); \
            out_count = entries.Len(); \
        }); \
    return true; \
}();
