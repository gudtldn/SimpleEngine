#pragma once

#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/TagTraits.h"
#include "SimpleEngine/ECS/ECSRegistry.h"


namespace se::detail
{
template <>
struct RegistrationTrait<se::meta::tags::Component>
{
    template <typename T>
    static void Apply()
    {
        ::se::ECSRegistry::Get().RegisterComponentOps<T>();
    }
};

template <>
struct RegistrationTrait<se::meta::tags::Resource>
{
    template <typename T>
    static void Apply()
    {
        ::se::ECSRegistry::Get().RegisterResourceOps<T>();
    }
};
} // namespace se::detail
