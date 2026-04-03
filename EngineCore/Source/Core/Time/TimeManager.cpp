#include "SimpleEngine/Core/Time/TimeManager.h"
#include "SimpleEngine/Utility/Debug.h"

#include <ranges>


namespace se
{
void TimeManager::Update(float raw_delta)
{
    // RealTime 갱신
    real_time.delta = raw_delta;
    real_time.elapsed += raw_delta;
    ++real_time.frame_count;

    // World별 GameTime/FixedTime 갱신
    for (auto& [game_time, fixed_time] : world_times | std::views::values)
    {
        game_time.delta = game_time.paused ? 0.0f : raw_delta * game_time.time_scale;
        game_time.elapsed += game_time.delta;
        ++game_time.frame_count;

        fixed_time.accumulator += game_time.delta;
    }
}

void TimeManager::RegisterWorld(const StringName& name)
{
    world_times.Entry(name).OrInsertWith([] { return WorldTimeState{}; });
}

void TimeManager::UnregisterWorld(const StringName& name)
{
    world_times.Remove(name);
}

GameTime& TimeManager::GetGameTime(const StringName& world_name)
{
    const auto state_opt = world_times.Find(world_name);
    SE_ASSERT(state_opt.HasValue(), "World '{}' is not registered in TimeManager.", world_name);
    return state_opt->game_time;
}

const GameTime& TimeManager::GetGameTime(const StringName& world_name) const
{
    const auto it = world_times.Find(world_name);
    SE_ASSERT(it, "World '{}' is not registered in TimeManager.", world_name);
    return it->game_time;
}

FixedTime& TimeManager::GetFixedTime(const StringName& world_name)
{
    const auto state_opt = world_times.Find(world_name);
    SE_ASSERT(state_opt.HasValue(), "World '{}' is not registered in TimeManager.", world_name);
    return state_opt->fixed_time;
}

const FixedTime& TimeManager::GetFixedTime(const StringName& world_name) const
{
    const auto state_opt = world_times.Find(world_name);
    SE_ASSERT(state_opt.HasValue(), "World '{}' is not registered in TimeManager.", world_name);
    return state_opt->fixed_time;
}
} // namespace se
