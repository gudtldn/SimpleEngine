#include "SimpleEngine/ECS/EntitySubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
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
    worlds.Clear();
}

void EntitySubsystem::PreUpdate()
{
    for (WorldContext& ctx : worlds | std::views::values)
    {
        ctx.RunPhase<PreUpdatePhase>();
    }
}

void EntitySubsystem::Update(double delta_time)
{
    for (auto& ctx : worlds | std::views::values)
    {
        World& world = ctx.GetWorld();

        // Time Resource를 갱신
        RealTime& real = world.GetResource<RealTime>();
        GameTime& game = world.GetResource<GameTime>();
        FixedTime& fixed = world.GetResource<FixedTime>();

        TimeManager::AdvanceRealTime(real, delta_time);
        TimeManager::AdvanceGameTime(game, delta_time);
        TimeManager::AccumulateFixedTime(fixed, game.GetDelta());

        // FixedUpdatePhase: accumulator 기반 반복 실행
        while (fixed.ConsumeStep())
        {
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

    // World 생성 시 시간 Resource를 최초 등록
    World& world = ctx.GetWorld();
    if (!world.HasResource<RealTime>())
    {
        world.InsertResource<RealTime>();
        world.InsertResource<GameTime>();
        world.InsertResource<FixedTime>();
    }

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

const StringName& EntitySubsystem::GetMainWorldName()
{
    static const StringName main_world_name = "Main";
    return main_world_name;
}
} // namespace se
