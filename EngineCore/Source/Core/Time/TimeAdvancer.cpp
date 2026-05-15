#include "SimpleEngine/Core/Time/TimeAdvancer.h"


namespace se
{
void TimeAdvancer::AdvanceRealTime(RealTime& real, f64 raw_delta)
{
    real.delta = raw_delta;
    real.elapsed += raw_delta;
    ++real.frame_count;
}

void TimeAdvancer::AdvanceGameTime(GameTime& game, f64 real_delta)
{
    if (game.paused)
    {
        game.delta = 0.0;
        return;
    }

    game.delta = real_delta * game.time_scale;
    game.elapsed += game.delta;
    ++game.frame_count;
}

void TimeAdvancer::AccumulateFixedTime(FixedTime& fixed, f64 game_delta)
{
    fixed.accumulator += game_delta;
}
} // namespace se
