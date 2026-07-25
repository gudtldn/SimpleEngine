#pragma once

#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/RegistrationHook.h"
#include "SimpleEngine/ECS/ECSRegistry.h"


namespace se::detail
{
template <>
struct RegistrationHook<se::meta::tags::Component>
{
    template <typename T>
    static void OnRegister()
    {
        ::se::ECSRegistry::Get().RegisterComponentOps<T>();
    }
};

template <>
struct RegistrationHook<se::meta::tags::Resource>
{
    template <typename T>
    static void OnRegister()
    {
        ::se::ECSRegistry::Get().RegisterResourceOps<T>();
    }
};
} // namespace se::detail
