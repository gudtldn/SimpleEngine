#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/ECS/Phases.h"
#include "SimpleEngine/ECS/Schedule.h"

#include <memory>


namespace se
{
// forward declaration
class World;

/**
 * ECS World와 Phase별 Schedule을 하나의 단위로 묶는 Context
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
     * @details 함수 시그니처를 분석하여 필요한 인자를(Query, World* 등)을 자동으로 주입받습니다.
     * @tparam P 시스템을 추가할 스케줄 타입 (예: PreUpdate, Update, PostUpdate)
     */
    template <PhaseType P, typename... Systems>
    Schedule& AddSystem(Systems&&... in_systems)
    {
        return schedules.Entry(TypeId::Get<P>()).OrDefault().Add(std::forward<Systems>(in_systems)...);
    }

    /**
     * Phase에 등록된 모든 시스템을 실행합니다.
     * 만약 등록된 시스템이 없으면 아무 동작도 하지 않습니다.
     */
    template <PhaseType P>
    void RunPhase()
    {
        if (const auto schedule = GetSchedule<P>())
        {
            schedule->Execute(world.get());
        }
    }

    /**
     * Phase의 Schedule을 반환합니다.
     * 만약, 등록된 시스템이 없으면 NullOpt를 반환합니다. */
    template <PhaseType P>
    Optional<Schedule&> GetSchedule()
    {
        return schedules.Find(TypeId::Get<P>());
    }

    [[nodiscard]] World& GetWorld() { return *world; }
    [[nodiscard]] const World& GetWorld() const { return *world; }

private:
    std::unique_ptr<World> world;
    HashMap<TypeId, Schedule> schedules;
};
} // namespace se
