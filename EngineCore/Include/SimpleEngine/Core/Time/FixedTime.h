#pragma once

#include "SimpleEngine/Core/Reflection/Traits.h"
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

    // TODO: C++26에서 std::meta::access_context::unchecked()로 접근하면 friend가 필요 없어짐
    friend struct TimeResources_Registrar;

public:
    /** 고정 시간 스텝(초)을 반환합니다. (기본값: 1/64) */
    [[nodiscard]] double GetFixedStep() const { return fixed_step; }

    /** 현재까지 누적된 시간(초)을 반환합니다. */
    [[nodiscard]] double GetAccumulator() const { return accumulator; }

    /** 보간 계수(0~1)를 반환합니다. (accumulator / fixed_step) */
    [[nodiscard]] double GetAlpha() const { return accumulator / fixed_step; }

    /** 고정 시간 스텝을 설정합니다. */
    void SetFixedStep(double step) { fixed_step = step; }

    /**
     * accumulator에서 고정 스텝 1회분을 소비합니다.
     * delta, elapsed, frame_count를 갱신하고, 소비 성공 여부를 반환합니다.
     */
    [[nodiscard]] bool ConsumeStep()
    {
        if (accumulator < fixed_step)
        {
            return false;
        }
        accumulator -= fixed_step;
        delta = fixed_step;
        elapsed += fixed_step;
        ++frame_count;
        return true;
    }

private:
    double fixed_step = 1.0 / 64.0;
    double accumulator = 0.0;
};
} // namespace se

SE_DECLARE_REFLECTION(se::FixedTime)
