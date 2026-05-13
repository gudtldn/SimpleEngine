#include "SimpleEngine/ECS/WorldContext.h"

#include "SimpleEngine/Core/Time/Time.h"
#include "SimpleEngine/Core/Time/TimeManager.h"
#include "SimpleEngine/ECS/TransformPropagation.h"
#include "SimpleEngine/ECS/World.h"

#include <ranges>


namespace se
{
WorldContext::WorldContext()
    : world(std::make_unique<World>())
{
    SetupDefaultStages();
}

WorldContext::~WorldContext() = default;

void WorldContext::RunAll(f64 delta_time)
{
    // 시간 리소스 갱신
    RealTime& real = world->GetResource<RealTime>();
    GameTime& game = world->GetResource<GameTime>();
    FixedTime& fixed = world->GetResource<FixedTime>();

    TimeManager::AdvanceRealTime(real, delta_time);
    TimeManager::AdvanceGameTime(game, delta_time);
    TimeManager::AccumulateFixedTime(fixed, game.GetDelta());

    // 모든 Stage를 순서대로 실행
    bool has_once_stages = false;

    for (ScheduleStage& stage : stages)
    {
        switch (stage.mode)
        {
        case EScheduleMode::Once:
            stage.schedule.Execute(*world);
            has_once_stages = true;
            break;

        case EScheduleMode::EveryFrame:
            stage.schedule.Execute(*world);
            break;

        case EScheduleMode::FixedTimestep:
            while (fixed.ConsumeStep())
            {
                stage.schedule.Execute(*world);
            }
            break;
        }
    }

    // 1회성 Stage가 존재했을 때만 제거
    if (has_once_stages)
    {
        stages.RemoveIf([](const ScheduleStage& s) { return s.mode == EScheduleMode::Once; });
    }
}

Optional<ScheduleStage&> WorldContext::FindStage(const TypeId& label)
{
    for (ScheduleStage& stage : stages)
    {
        if (stage.label == label)
        {
            return stage;
        }
    }
    return NullOpt;
}

void WorldContext::InsertStageAfter(const TypeId& anchor, const TypeId& label, EScheduleMode mode)
{
    SE_ASSERT(!FindStage(label).HasValue(), "InsertStageAfter: Stage already exists.");

    for (const auto [idx, stage] : stages | std::views::enumerate)
    {
        if (stage.label == anchor)
        {
            stages.Insert(static_cast<usize>(idx) + 1, {
                .label = label,
                .schedule = Schedule{},
                .mode = mode
            });
            return;
        }
    }

    SE_ASSERT(false, "InsertStageAfter: Anchor stage not found.");
}

void WorldContext::InsertStageBefore(const TypeId& anchor, const TypeId& label, EScheduleMode mode)
{
    SE_ASSERT(!FindStage(label).HasValue(), "InsertStageBefore: Stage already exists.");

    for (const auto [idx, stage] : stages | std::views::enumerate)
    {
        if (stage.label == anchor)
        {
            stages.Insert(static_cast<usize>(idx), {
                .label = label,
                .schedule = Schedule{},
                .mode = mode
            });
            return;
        }
    }

    SE_ASSERT(false, "InsertStageBefore: Anchor stage not found.");
}

void WorldContext::SetupDefaultStages()
{
    stages.Push({ .label = TypeId::Get<StartupPhase>(),     .schedule = Schedule{}, .mode = EScheduleMode::Once });
    stages.Push({ .label = TypeId::Get<PreUpdatePhase>(),   .schedule = Schedule{}, .mode = EScheduleMode::EveryFrame });
    stages.Push({ .label = TypeId::Get<FixedUpdatePhase>(), .schedule = Schedule{}, .mode = EScheduleMode::FixedTimestep });
    stages.Push({ .label = TypeId::Get<UpdatePhase>(),      .schedule = Schedule{}, .mode = EScheduleMode::EveryFrame });
    stages.Push({ .label = TypeId::Get<PostUpdatePhase>(),  .schedule = Schedule{}, .mode = EScheduleMode::EveryFrame });

    // GlobalTransform 자동 추가 + 계층 전파 (PostUpdate에서 자동 실행)
    AddSystem<PostUpdatePhase>(SyncGlobalTransforms, PropagateTransforms);
}
} // namespace se
