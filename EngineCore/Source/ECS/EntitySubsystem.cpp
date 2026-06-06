#include "SimpleEngine/ECS/EntitySubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/ECSRegistry.h"

#include <ranges>


namespace se
{
SE_REGISTER_SUBSYSTEM(EntitySubsystem);

SE_BEGIN_REFLECT(EntitySubsystem, meta::Reflect, meta::Hidden, meta::Transient)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(EntitySubsystem)

bool EntitySubsystem::Initialize()
{
    GetOrCreateWorld(GetMainWorldName());
    return true;
}

void EntitySubsystem::Release()
{
    worlds.Clear();
}

void EntitySubsystem::Update(f64 delta_time)
{
    for (WorldContext& ctx : worlds | std::views::values)
    {
        ctx.RunAll(delta_time);
    }
}

WorldContext& EntitySubsystem::GetOrCreateWorld(const StringName& name)
{
    WorldContext& ctx = worlds.Entry(name).OrInsertWith([] { return WorldContext{}; });

    // Transient 리소스 (시간 등)를 최초 등록
    ECSRegistry::Get().InsertDefaultTransientResources(ctx.GetWorld());

    return ctx;
}

Optional<WorldContext&> EntitySubsystem::FindWorld(const StringName& name)
{
    return worlds.Find(name);
}

WorldContext& EntitySubsystem::GetMainWorld()
{
    return worlds.FindChecked(GetMainWorldName());
}

const WorldContext& EntitySubsystem::GetMainWorld() const
{
    return worlds.FindChecked(GetMainWorldName());
}

void EntitySubsystem::DestroyWorld(const StringName& name)
{
    worlds.Remove(name);
}

HashMap<StringName, WorldContext>& EntitySubsystem::GetWorlds()
{
    return worlds;
}

const HashMap<StringName, WorldContext>& EntitySubsystem::GetWorlds() const
{
    return worlds;
}

const StringName& EntitySubsystem::GetMainWorldName()
{
    static const StringName main_world_name = "Main";
    return main_world_name;
}
} // namespace se
