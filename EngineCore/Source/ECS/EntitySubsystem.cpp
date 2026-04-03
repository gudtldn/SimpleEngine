#include "SimpleEngine/ECS/EntitySubsystem.h"

#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/Core/Time/FixedTime.h"
#include "SimpleEngine/Core/Time/GameTime.h"
#include "SimpleEngine/Core/Time/RealTime.h"
#include "SimpleEngine/Core/Time/TimeManager.h"
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
    TimeManager& time_manager = Engine::Get().GetTimeManager();
    for (const auto& name : worlds | std::views::keys)
    {
        time_manager.UnregisterWorld(name);
    }
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
    (void)delta_time;

    TimeManager& time_manager = Engine::Get().GetTimeManager();
    for (auto& [name, ctx] : worlds)
    {
        World& world = ctx.GetWorld();

        // World별 시간 Resource 주입
        world.InsertResource<RealTime>(time_manager.GetRealTime());
        world.InsertResource<GameTime>(time_manager.GetGameTime(name));
        world.InsertResource<FixedTime>(time_manager.GetFixedTime(name));

        // FixedUpdatePhase: accumulator 기반 반복 실행
        FixedTime& fixed = time_manager.GetFixedTime(name);
        while (fixed.ConsumeStep())
        {
            world.InsertResource<FixedTime>(fixed);
            ctx.RunPhase<FixedUpdatePhase>();
        }

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
    WorldContext& ctx = worlds.Entry(name).OrInsertWith([] { return WorldContext{}; });
    Engine::Get().GetTimeManager().RegisterWorld(name);
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
    Engine::Get().GetTimeManager().UnregisterWorld(name);
}

const StringName& EntitySubsystem::GetMainWorldName()
{
    static const StringName main_world_name = "Main";
    return main_world_name;
}
} // namespace se
