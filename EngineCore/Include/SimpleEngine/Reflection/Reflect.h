#pragma once
#include <concepts>
#include <cstddef>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Reflection/Meta.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Reflection/TypeRegistry.h"
#include "SimpleEngine/Utility/Common.h"


// TODO: C++26되면 여기 파일 전체 수정해야 함

/** TODO: docs */
#define SE_TYPE_ANNOTATION(...)
/** TODO: docs */
#define SE_PROPERTY_ANNOTATION(...)

/** 타입의 리플렉션 정보 등록을 시작합니다. */
#define SE_BEGIN_REFLECT(type) \
namespace se::refl::registration \
{ \
inline static const struct type##_Registrar \
{ \
    type##_Registrar() \
    { \
        using T = type; \
        se::Array<se::refl::PropertyInfo> properties;

/** 멤버 변수를 기본 메타데이터로 리플렉션에 등록합니다. */ // TODO: 추후 offset 대신 멤버 포인터를 저장
#define SE_REFLECT_PROPERTY(property_name) \
        properties.Emplace( \
            /* .name     = */ SE_STRINGIFY(property_name), \
            /* .size     = */ sizeof(T::property_name), \
            /* .offset   = */ offsetof(T, property_name), \
            /* .type_id  = */ se::refl::TypeId::Get<decltype(T::property_name)>(), \
            /* .metadata = */ se::refl::PropertyMetadata{} \
        );

/**
 * 멤버 변수를 사용자 정의 메타데이터와 함께 리플렉션에 등록합니다.
 * @param property_name 멤버 변수의 이름
 * @param ...           PropertyMetadata 초기화 구문 (예: .display_name = u8"이름")
 */
#define SE_REFLECT_PROPERTY_WITH_META(property_name, ...) \
        properties.Emplace( \
            /* .name     = */ SE_STRINGIFY(property_name), \
            /* .size     = */ sizeof(T::property_name), \
            /* .offset   = */ offsetof(T, property_name), \
            /* .type_id  = */ se::refl::TypeId::Get<decltype(T::property_name)>(), \
            /* .metadata = */ se::refl::PropertyMetadata{__VA_ARGS__} \
        );

/** 타입의 리플렉션 정보 등록을 마칩니다. */
#define SE_END_REFLECT(type) \
        se::refl::TypeInfo info{ \
            .name = SE_STRINGIFY(type), \
            .size = sizeof(type), \
            .properties = std::move(properties), \
            .type_id = se::refl::TypeId::Get<type>(), \
        }; \
        se::refl::TypeRegistry::GetInstance().RegisterType(std::move(info)); \
    } \
} SE_UNIQUE_TOKEN(SE_CONCAT_TOKEN(type, _Registrar)){}; \
} // se::refl::registration
