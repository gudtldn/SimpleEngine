// ReSharper disable CppUnusedIncludeDirective
#pragma once
#include <concepts>

#include "SimpleEngine/Core/Math/MathSerialize.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/ECS/ComponentRegistry.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Common.h"


// TODO: C++26되면 여기 파일 전체 수정해야 함

namespace se::detail
{
template <typename T, typename... Tags>
consteval BitFlags<ETypeFlags> MakeTypeFlags(Tags&&... tags)
{
    using namespace se;
    using namespace se::meta::detail;

    BitFlags<ETypeFlags> flags;

    // Abstract 클래스 자동 감지
    if constexpr (std::is_abstract_v<T>)
    {
        flags |= ETypeFlags::Abstract;
    }

    auto process_tag = [&flags]<typename Tag>([[maybe_unused]] Tag&& tag)
    {
        using DecayedTag = std::decay_t<Tag>;

        // ECS Tags
        if constexpr (std::same_as<DecayedTag, ComponentTag>)
        {
            flags |= ETypeFlags::Component;
        }
        else if constexpr (std::same_as<DecayedTag, ResourceTag>)
        {
            flags |= ETypeFlags::Resource;
        }
        else if constexpr (std::same_as<DecayedTag, EventTag>)
        {
            flags |= ETypeFlags::Event;
        }

        // General Tags
        else if constexpr (std::same_as<DecayedTag, TransientTag>)
        {
            flags |= ETypeFlags::Transient;
        }
        else
        {
            static_assert(se::traits::AlwaysFalse<DecayedTag>, "Invalid type tag");
        }
    };

    (process_tag(std::forward<Tags>(tags)), ...);
    return flags;
}

template <typename... Tags>
consteval PropertyMetadata MakePropertyMetadata(Tags&&... tags)
{
    using namespace se;
    using namespace se::meta;
    using namespace se::meta::detail;

    PropertyMetadata meta{};
    auto process_tag = [&meta]<typename Tag>(Tag&& tag)
    {
        using DecayedTag = std::decay_t<Tag>;

        // --- 1. Marker Tags (Flags) ---
        if constexpr (std::same_as<DecayedTag, EditTag>)
        {
            meta.flags |= EPropertyFlags::DefaultEdit;
        }
        else if constexpr (std::same_as<DecayedTag, ReadOnlyTag>)
        {
            meta.flags |= EPropertyFlags::DefaultReadOnly;
        }
        else if constexpr (std::same_as<DecayedTag, SerializeTag>)
        {
            meta.flags |= EPropertyFlags::Serialized;
        }
        else if constexpr (std::same_as<DecayedTag, TransientTag>)
        {
            // Transient 추가 및 Serialized 제거
            meta.flags |= EPropertyFlags::Transient;
            meta.flags = meta.flags & ~EPropertyFlags::Serialized;
        }
        else if constexpr (std::same_as<DecayedTag, ColorTag>)
        {
            meta.flags |= EPropertyFlags::ColorPicker;
        }

        // --- 2. Payload Tags (Data) ---
        else if constexpr (std::same_as<DecayedTag, Range>)
        {
            meta.has_range = true;
            meta.range_min = tag.min;
            meta.range_max = tag.max;
        }
        else if constexpr (std::same_as<DecayedTag, Tooltip>)
        {
            meta.tooltip = tag.message;
        }
        else if constexpr (std::same_as<DecayedTag, DisplayName>)
        {
            meta.display_name = tag.name;
        }
        else
        {
            static_assert(se::traits::AlwaysFalse<DecayedTag>, "Invalid property tag");
        }
    };

    (process_tag(std::forward<Tags>(tags)), ...);
    return meta;
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
        return ::se::TypeId::Get<this_class>(); \
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
    constexpr auto type_flags = ::se::detail::MakeTypeFlags<T>(__VA_ARGS__); \
    ::se::TypeRegistry::Get().Register<T>() \
        .AddFlags(type_flags)

/**
 * 인터페이스(Interface)를 등록합니다.
 * @param ... 인터페이스 목록
 */
#define SE_REFLECT_INTERFACE(...) \
        .Implements<__VA_ARGS__>()

/**
 * 멤버 변수(Property)를 등록합니다.
 * @param member 멤버 변수
 * @param ... 프로퍼티 속성 태그
 */
#define SE_REFLECT_PROPERTY(member, ...) \
        .Property<&T::member>(SE_STRINGIFY(member)) \
        .ApplyMetadata(::se::detail::MakePropertyMetadata(__VA_ARGS__))

/** 타입의 리플렉션 정보 등록을 마칩니다. */
#define SE_END_REFLECT(type) \
    ; /* 체이닝 종료 */ \
    static_assert(std::same_as<std::decay_t<T>, std::decay_t<type>>, "Type mismatch between BEGIN and END reflect macros."); \
    if constexpr (type_flags.IsAnySet(::se::ETypeFlags::Component)) \
    { \
        ::se::ecs::ComponentRegistry::Get().RegisterInterface<type>(); \
    } \
    return true; \
}();

/**
 * 열거형(Enum)의 리플렉션 정보를 등록합니다.
 * underlying type의 TypeId가 자동으로 base_or_inner_id에 설정되며,
 * underlying type 기반 직렬화 콜백이 자동 등록됩니다.
 *
 * @note 이 매크로를 사용하는 파일에서 Archive.h가 포함되어야 합니다.
 * @param enum_type 등록할 열거형 이름
 */
#define SE_REFLECT_ENUM(enum_type) \
inline static const bool SE_CONCAT_NAME(_Reflect_Init_Enum_, enum_type) = [] static -> bool \
{ \
    using T = enum_type; \
    ::se::TypeRegistry::Get().RegisterEnum<T>() \
        .Serialize([](::se::Archive& ar, void* p) static { ar << *static_cast<T*>(p); }); \
    return true; \
}();
