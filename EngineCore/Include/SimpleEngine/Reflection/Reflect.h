#pragma once
#if false
#include <concepts>
#include <cstddef>

#include "SimpleEngine/Reflection/Meta.h"
#include "SimpleEngine/Reflection/ReflectionRegistry.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Utility/Common.h"


// TODO: C++26되면 여기 파일 전체 수정해야 함

namespace se::reflection
{
template <typename T>
    requires requires { { T::Reflect() } -> std::same_as<const TypeInfo&>; }
const TypeInfo& GetTypeInfo()
{
    return T::Reflect();
}

namespace details
{
template <typename T>
struct TypeRegistrar
{
    TypeRegistrar()
    {
        ReflectionRegistry::GetInstance().RegisterType(&GetTypeInfo<T>());
    }
};
}
}

/** (.h용) 리플렉션을 적용할 타입의 선언부 안에 사용합니다. */
#define SE_REFLECTABLE(...) \
private: \
    static se::reflection::TypeInfo RegisterReflection(); \
public: \
    static const se::reflection::TypeInfo& Reflect();

/** (.h용) 리플렉션을 적용할 프로퍼티 위에 사용합니다. */
#define SE_PROPERTY(...)

/** (.cpp용) 타입의 리플렉션 정보 등록을 시작합니다. */
#define SE_BEGIN_REFLECT(type) \
se::reflection::TypeInfo type::RegisterReflection() \
{ \
    using T = type; \
    std::vector<se::reflection::PropertyInfo> properties;

/** (.cpp용) 멤버 변수를 기본 메타데이터로 리플렉션에 등록합니다. */
#define SE_REFLECT_PROPERTY(property_name) \
    properties.emplace_back( \
        /* .name     = */ SE_STRINGIFY(property_name), \
        /* .size     = */ sizeof(T::property_name), \
        /* .offset   = */ offsetof(T, property_name), \
        /* .type_id  = */ se::reflection::TypeId::Get<decltype(T::property_name)>(), \
        /* .metadata = */ se::reflection::PropertyMetadata{} \
    );

/**
 * (.cpp용) 멤버 변수를 사용자 정의 메타데이터와 함께 리플렉션에 등록합니다.
 * @param property_name 멤버 변수의 이름
 * @param ...           PropertyMetadata 초기화 구문 (예: .display_name = u8"이름")
 */
#define SE_REFLECT_PROPERTY_WITH_META(property_name, ...) \
    properties.emplace_back( \
        /* .name     = */ SE_STRINGIFY(property_name), \
        /* .size     = */ sizeof(T::property_name), \
        /* .offset   = */ offsetof(T, property_name), \
        /* .type_id  = */ se::reflection::TypeId::Get<decltype(T::property_name)>(), \
        /* .metadata = */ se::reflection::PropertyMetadata{__VA_ARGS__} \
    );

/** (.cpp용) 타입의 리플렉션 정보 등록을 마칩니다. */
#define SE_END_REFLECT(type) \
    return se::reflection::TypeInfo{ \
        .name = SE_STRINGIFY(type), \
        .size = sizeof(type), \
        .properties = std::move(properties), \
        .type_id = se::reflection::TypeId::Get<type>(), \
    }; \
} \
const se::reflection::TypeInfo& type::Reflect() \
{ \
    static const se::reflection::TypeInfo instance = RegisterReflection(); \
    return instance; \
} \
namespace \
{ \
    static const se::reflection::details::TypeRegistrar<type> SE_UNIQUE_TOKEN(SE_CONCAT_TOKEN(registrar_, type)); \
}
#endif
