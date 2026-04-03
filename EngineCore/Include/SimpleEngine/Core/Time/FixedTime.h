#pragma once

#include "SimpleEngine/Core/Time/TimeTick.h"


namespace se
{
/**
 * 고정 시간 스텝을 나타냅니다. World별로 독립적으로 관리됩니다.
 * 물리 시뮬레이션 등 일정한 간격의 업데이트가 필요한 곳에서 사용합니다.
 * accumulator가 fixed_step 이상이면 FixedUpdatePhase가 실행됩니다.
 */
class FixedTime final : public TimeTick
{
    friend class TimeManager;

public:
    /** 고정 시간 스텝(초)을 반환합니다. (기본값: 1/64) */
    [[nodiscard]] float GetFixedStep() const { return fixed_step; }

    /** 현재까지 누적된 시간(초)을 반환합니다. */
    [[nodiscard]] double GetAccumulator() const { return accumulator; }

    /** 고정 시간 스텝을 설정합니다. */
    void SetFixedStep(float step) { fixed_step = step; }

    /**
     * accumulator에서 고정 스텝 1회분을 소비합니다.
     * delta, elapsed, frame_count를 갱신하고, 소비 성공 여부를 반환합니다.
     */
    [[nodiscard]] bool ConsumeStep()
    {
        if (accumulator < static_cast<double>(fixed_step))
        {
            return false;
        }
        accumulator -= static_cast<double>(fixed_step);
        delta = fixed_step;
        elapsed += fixed_step;
        ++frame_count;
        return true;
    }

private:
    float fixed_step = 1.0f / 64.0f;
    double accumulator = 0.0;
};
} // namespace se
