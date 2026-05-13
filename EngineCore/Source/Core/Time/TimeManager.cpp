#include "SimpleEngine/Core/Time/TimeManager.h"


namespace se
{
void TimeManager::AdvanceRealTime(RealTime& real, f64 raw_delta)
{
    real.delta = raw_delta;
    real.elapsed += raw_delta;
    ++real.frame_count;
}

void TimeManager::AdvanceGameTime(GameTime& game, f64 raw_delta)
{
    if (game.paused)
    {
        game.delta = 0.0;
        return;
    }

    game.delta = raw_delta * game.time_scale;
    game.elapsed += game.delta;
    ++game.frame_count;
}

void TimeManager::AccumulateFixedTime(FixedTime& fixed, f64 game_delta)
{
    fixed.accumulator += game_delta;
}
} // namespace se
