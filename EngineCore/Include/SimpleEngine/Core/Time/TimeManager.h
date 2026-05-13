#pragma once

#include "SimpleEngine/Core/Time/Time.h"


namespace se
{
/**
 * 시간 데이터의 갱신 로직을 담당하는 무상태(stateless) 클래스
 * 데이터를 소유하지 않으며, World가 Resource로 소유하는 시간 객체를 매 프레임 friend 권한으로 갱신합니다.
 */
class SE_CORE_API TimeManager
{
public:
    TimeManager() = delete;

    /** RealTime을 raw_delta 기준으로 갱신합니다. */
    static void AdvanceRealTime(RealTime& real, f64 raw_delta);

    /** GameTime을 raw_delta 기준으로 갱신합니다. pause/time_scale이 적용됩니다. */
    static void AdvanceGameTime(GameTime& game, f64 raw_delta);

    /**
     * FixedTime의 accumulator를 game_delta만큼 누적합니다.
     * 실제 소비는 FixedTime::ConsumeStep()으로 수행합니다.
     */
    static void AccumulateFixedTime(FixedTime& fixed, f64 game_delta);
};
} // namespace se
