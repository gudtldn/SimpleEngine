#pragma once

#include "../Core/Reflection/Legacy/Annotations.h"
#include "../Core/Reflection/Legacy/TagTraits.h"
#include "SimpleEngine/ECS/ECSRegistry.h"


namespace se::detail
{
template <>
struct RegistrationTrait_v1<se::meta::tags::Component>
{
    template <typename T>
    static void Apply()
    {
        ::se::ECSRegistry::Get().RegisterComponentOps<T>();
    }
};

template <>
struct RegistrationTrait_v1<se::meta::tags::Resource>
{
    template <typename T>
    static void Apply()
    {
        ::se::ECSRegistry::Get().RegisterResourceOps<T>();
    }
};
} // namespace se::detail
