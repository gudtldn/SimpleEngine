#include "SimpleEngine/ECS/EntitySubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/Phases.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>


namespace se
{
SE_REGISTER_SUBSYSTEM(EntitySubsystem);

SE_BEGIN_REFLECT(EntitySubsystem, meta::Internal)
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

void EntitySubsystem::PreUpdate()
{
    for (WorldContext& ctx : worlds | std::views::values)
    {
        ctx.RunPhase<PreUpdatePhase>();
    }
}

void EntitySubsystem::Update(float delta_time)
{
    // TODO: Time Resource 주입하도록 수정
    (void)delta_time;

    for (WorldContext& ctx : worlds | std::views::values)
    {
        ctx.RunPhase<UpdatePhase>();
    }
}

void EntitySubsystem::PostUpdate()
{
    for (WorldContext& ctx : worlds | std::views::values)
    {
        ctx.RunPhase<PostUpdatePhase>();
    }
}

WorldContext& EntitySubsystem::GetOrCreateWorld(const StringName& name)
{
    return worlds.Entry(name).OrInsertWith([]{ return WorldContext{}; });
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

const StringName& EntitySubsystem::GetMainWorldName()
{
    static const StringName main_world_name = "Main";
    return main_world_name;
}
} // namespace se
