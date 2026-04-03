#include "SimpleEngine/Core/Time/TimeManager.h"

#include "SimpleEngine/Core/Time/FixedTime.h"
#include "SimpleEngine/Core/Time/GameTime.h"
#include "SimpleEngine/Core/Time/RealTime.h"


namespace se
{
void TimeManager::AdvanceRealTime(RealTime& real, double raw_delta)
{
    real.delta = raw_delta;
    real.elapsed += raw_delta;
    ++real.frame_count;
}

void TimeManager::AdvanceGameTime(GameTime& game, double raw_delta)
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

void TimeManager::AdvanceFixedTime(FixedTime& fixed, double game_delta)
{
    fixed.accumulator += game_delta;
}
} // namespace se
