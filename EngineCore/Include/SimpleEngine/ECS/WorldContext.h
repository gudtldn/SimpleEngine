#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/ECS/Phases.h"
#include "SimpleEngine/ECS/Schedule.h"

#include <memory>


namespace se
{
// forward declaration
class World;

/**
 * Schedule Stage: Phase 라벨, 스케줄, 실행 모드를 하나로 묶는 단위
 */
struct ScheduleStage
{
    TypeId label;
    Schedule schedule;
    EScheduleMode mode;
};

/**
 * ECS World와 Stage별 Schedule을 하나의 단위로 묶는 Context
 */
class SE_CORE_API WorldContext
{
public:
    WorldContext();
    ~WorldContext();

    // 이동만 허용
    WorldContext(WorldContext&&) = default;
    WorldContext& operator=(WorldContext&&) = default;
    WorldContext(const WorldContext&) = delete;
    WorldContext& operator=(const WorldContext&) = delete;

public:
    /**
     * 함수, System, SystemChain 등을 스케줄에 추가합니다.
     * @details 함수 시그니처를 분석하여 필요한 인자를(Query, Commands 등)을 자동으로 주입받습니다.
     * @tparam P 시스템을 추가할 Phase 타입 (예: StartupPhase, PreUpdatePhase, UpdatePhase, PostUpdatePhase)
     * @note Phase에 해당하는 Stage가 반드시 먼저 등록되어 있어야 합니다.
     */
    template <PhaseType P, typename... Systems>
    Schedule& AddSystem(Systems&&... in_systems)
    {
        const auto stage = FindStage(TypeId::Get<P>());
        SE_ASSERT(stage.HasValue(), "AddSystem: Stage not found. Register it first with AddStageAfter/AddStageBefore.");
        return stage->schedule.Add(std::forward<Systems>(in_systems)...);
    }

    /**
     * 새로운 Stage를 기존 Stage 뒤에 삽입합니다.
     * @tparam After 기준이 되는 기존 Phase 타입
     * @tparam P 새로 추가할 Phase 타입
     * @param mode 실행 모드 (Once, EveryFrame, FixedTimestep)
     */
    template <PhaseType After, PhaseType P>
    void AddStageAfter(EScheduleMode mode = EScheduleMode::EveryFrame)
    {
        InsertStageAfter(TypeId::Get<After>(), TypeId::Get<P>(), mode);
    }

    /**
     * 새로운 Stage를 기존 Stage 앞에 삽입합니다.
     * @tparam Before 기준이 되는 기존 Phase 타입
     * @tparam P 새로 추가할 Phase 타입
     * @param mode 실행 모드 (Once, EveryFrame, FixedTimestep)
     */
    template <PhaseType Before, PhaseType P>
    void AddStageBefore(EScheduleMode mode = EScheduleMode::EveryFrame)
    {
        InsertStageBefore(TypeId::Get<Before>(), TypeId::Get<P>(), mode);
    }

    /**
     * 지정된 Phase의 Schedule만 단독 실행합니다.
     * 만약 등록된 시스템이 없으면 아무 동작도 하지 않습니다.
     */
    template <PhaseType P>
    void RunPhase()
    {
        if (const auto stage = FindStage(TypeId::Get<P>()))
        {
            stage->schedule.Execute(*world);
        }
    }

    /**
     * 모든 Stage를 순서대로 실행합니다.
     * @param delta_time 이전 프레임과의 시간 간격 (초)
     */
    void RunAll(double delta_time);

    [[nodiscard]] World& GetWorld() { return *world; }
    [[nodiscard]] const World& GetWorld() const { return *world; }

private:
    /** TypeId에 해당하는 Stage를 찾습니다. 없으면 NullOpt. */
    Optional<ScheduleStage&> FindStage(const TypeId& label);

    /** anchor Stage 뒤에 새 Stage를 삽입합니다. */
    void InsertStageAfter(const TypeId& anchor, const TypeId& label, EScheduleMode mode);

    /** anchor Stage 앞에 새 Stage를 삽입합니다. */
    void InsertStageBefore(const TypeId& anchor, const TypeId& label, EScheduleMode mode);

    /** 기본 Stage 순서를 초기화합니다. */
    void SetupDefaultStages();

private:
    std::unique_ptr<World> world;
    Array<ScheduleStage> stages;
};
} // namespace se
