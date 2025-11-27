#pragma once
#include <concepts>
#include <cstddef>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Reflection/Annotations.h"
#include "SimpleEngine/Reflection/Meta.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Reflection/TypeRegistry.h"
#include "SimpleEngine/Utility/Common.h"


// TODO: C++26되면 여기 파일 전체 수정해야 함

namespace se::refl::details
{
template <typename... Tags>
consteval PropertyMetadata MakePropertyMetadata(Tags&&... tags)
{
    using namespace se::meta;
    using namespace se::meta::details;

    PropertyMetadata meta{};

    auto process_tag = [&meta]<typename Tag>(Tag&& tag)
    {
        using T = std::decay_t<Tag>;

        // --- 1. Marker Tags (Flags) ---
        if constexpr (std::same_as<T, EditTag>)
        {
            meta.flags |= EPropertyFlags::DefaultEdit;
        }
        else if constexpr (std::same_as<T, ReadOnlyTag>)
        {
            meta.flags |= EPropertyFlags::DefaultReadOnly;
        }
        else if constexpr (std::same_as<T, SerializeTag>)
        {
            meta.flags |= EPropertyFlags::Serialized;
        }
        else if constexpr (std::same_as<T, TransientTag>)
        {
            // Transient 추가 및 Serialized 제거
            meta.flags |= EPropertyFlags::Transient;
            meta.flags = meta.flags & ~EPropertyFlags::Serialized;
        }
        else if constexpr (std::same_as<T, ColorTag>)
        {
            meta.flags |= EPropertyFlags::ColorPicker;
        }

        // --- 2. Payload Tags (Data) ---
        else if constexpr (std::same_as<T, Range>)
        {
            meta.has_range = true;
            meta.range_min = tag.min;
            meta.range_max = tag.max;
        }
        else if constexpr (std::same_as<T, Tooltip>)
        {
            meta.tooltip = tag.message;
        }
        else if constexpr (std::same_as<T, DisplayName>)
        {
            meta.display_name = tag.name;
        }
    };

    (process_tag(std::forward<Tags>(tags)), ...);
    return meta;
}
}

/** 타입의 리플렉션 정보 등록을 시작합니다. */
#define SE_BEGIN_REFLECT(type) \
namespace se::refl::registration \
{ \
inline static const struct type##_Registrar \
{ \
    type##_Registrar() \
    { \
        using T = type; \
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
            .properties = std::move(properties), \
            .type_id = ::se::refl::TypeId::Get<type>(), \
        }); \
    } \
} SE_UNIQUE_TOKEN(SE_CONCAT_TOKEN(type, _Registrar)){}; \
} // se::refl::registration
