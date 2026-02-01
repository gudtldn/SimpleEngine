#pragma once
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::ecs
{
/**
 * @todo docs
 */
class SE_CORE_API ComponentRegistry
{
public:
    using StorageFactory = Function<void(World&)>;

    static ComponentRegistry& Get()
    {
        static ComponentRegistry instance;
        return instance;
    }

    template <typename T>
    static void Register()
    {
        Get().factories.Emplace(TypeId::Get<T>(), [](World& world)
        {
            world.GetOrCreateStorage<T>();
        });
    }

    bool EnsureStorage(World& world, const TypeId& type_id) const
    {
        if (const Optional func_opt = factories.Find(type_id))
        {
            func_opt->Invoke(world);
            return true;
        }
        return false;
    }

private:
    HashMap<TypeId, StorageFactory> factories;
};
}  // namespace se::ecs
