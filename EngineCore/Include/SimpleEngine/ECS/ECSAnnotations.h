#pragma once

#include "SimpleEngine/Core/Reflection/RegistrationTraits.h"
#include "SimpleEngine/ECS/ECSRegistry.h"


namespace se::ecs
{
struct ComponentAnnotation{};
inline constexpr ComponentAnnotation Component{};

struct ResourceAnnotation{};
inline constexpr ResourceAnnotation Resource{};
} // namespace se::ecs

template <>
struct se::RegistrationTraits<se::ecs::ComponentAnnotation>
{
    template <typename T>
    static void OnTypeRegistered(const ecs::ComponentAnnotation&)
    {
        ECSRegistry::Get().RegisterComponentOps<T>();
    }
};

template <>
struct se::RegistrationTraits<se::ecs::ResourceAnnotation>
{
    template <typename T>
    static void OnTypeRegistered(const ecs::ResourceAnnotation&)
    {
        ECSRegistry::Get().RegisterResourceOps<T>();
    }
};
