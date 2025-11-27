#pragma once
#include <concepts>
#include <cstddef>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Reflection/Annotations.h"
#include "SimpleEngine/Reflection/Meta.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Reflection/TypeRegistry.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Common.h"


// TODO: C++26되면 여기 파일 전체 수정해야 함

namespace se::refl::details
{
template <typename T, typename... Tags>
consteval BitFlags<ETypeFlags> MakeTypeFlags(Tags&&... tags)
{
    using namespace se::meta;
    using namespace se::meta::details;

    BitFlags<ETypeFlags> flags;
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
        else if constexpr (std::is_abstract_v<T>)
        {
            flags |= ETypeFlags::Abstract;
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
    using namespace se::meta;
    using namespace se::meta::details;

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
}

/** 타입의 리플렉션 정보 등록을 시작합니다. */
#define SE_BEGIN_REFLECT(type, ...) \
namespace se::refl::registration \
{ \
inline static const struct type##_Registrar \
{ \
    type##_Registrar() \
    { \
        using T = type; \
        constexpr auto type_flags = ::se::refl::details::MakeTypeFlags<T>(__VA_ARGS__); \
        ::se::Array<::se::refl::PropertyInfo> properties;

/** 멤버 변수를 기본 메타데이터로 리플렉션에 등록합니다. */ // TODO: 추후 offset 대신 멤버 포인터를 저장
#define SE_REFLECT_PROPERTY(property_name, ...) \
        properties.Emplace( \
            /* .name     = */ SE_STRINGIFY(property_name), \
            /* .size     = */ sizeof(T::property_name), \
            /* .offset   = */ offsetof(T, property_name), \
            /* .type_id  = */ ::se::refl::TypeId::Get<decltype(T::property_name)>(), \
            /* .metadata = */ ::se::refl::details::MakePropertyMetadata(__VA_ARGS__) \
        );

/** 타입의 리플렉션 정보 등록을 마칩니다. */
#define SE_END_REFLECT(type) \
        ::se::refl::TypeRegistry::GetInstance().RegisterType({ \
            .name = ::se::refl::GetFullTypeName<type>(), \
            .size = sizeof(type), \
            .flags = type_flags, \
            .properties = std::move(properties), \
            .type_id = ::se::refl::TypeId::Get<type>(), \
        }); \
    } \
} SE_UNIQUE_TOKEN(SE_CONCAT_TOKEN(type, _Registrar)){}; \
} // se::refl::registration
