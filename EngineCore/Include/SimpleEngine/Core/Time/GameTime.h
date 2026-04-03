#pragma once

#include "SimpleEngine/Core/Time/TimeTick.h"


namespace se
{
/**
 * 게임 시간을 나타냅니다. World별로 독립적으로 관리됩니다.
 * time_scale과 pause 상태에 따라 delta가 조절됩니다.
 */
class GameTime final : public TimeTick
{
    friend class TimeManager;

public:
    /** 현재 시간 배율을 반환합니다. (기본값: 1.0) */
    [[nodiscard]] float GetTimeScale() const { return time_scale; }

    /** 일시정지 상태인지 반환합니다. */
    [[nodiscard]] bool IsPaused() const { return paused; }

    /** 시간 배율을 설정합니다. */
    void SetTimeScale(float scale) { time_scale = scale; }

    /** 일시정지 상태를 설정합니다. */
    void SetPaused(bool pause) { paused = pause; }

private:
    float time_scale = 1.0f;
    bool paused = false;
};
} // namespace se
