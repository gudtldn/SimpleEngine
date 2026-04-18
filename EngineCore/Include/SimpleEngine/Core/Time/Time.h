#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/Traits.h"


namespace se
{
// forward declarations
class TimeManager;
struct TimeResources_Registrar;

namespace detail
{
/**
 * 시간 데이터의 공통 클래스
 * delta(프레임 경과), elapsed(누적 경과), frame_count를 제공합니다.
 */
class TimeTick
{
    friend class ::se::TimeManager;

    // TODO: C++26에서 std::meta::access_context::unchecked()로 접근하면 friend가 필요 없어짐
    friend struct ::se::TimeResources_Registrar;

public:
    /** 이번 프레임 경과 시간(초)을 반환합니다. */
    [[nodiscard]] double GetDelta() const { return delta; }

    /** 누적 경과 시간(초)을 반환합니다. */
    [[nodiscard]] double GetElapsed() const { return elapsed; }

    /** 현재까지의 프레임 카운트를 반환합니다. */
    [[nodiscard]] uint64 GetFrameCount() const { return frame_count; }

protected:
    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    double delta = 0.0;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    double elapsed = 0.0;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    uint64 frame_count = 0;
};
} // namespace detail

/**
 * 실제 글로벌 시간입니다.
 * 게임 일시정지나 time scale의 영향을 받지 않습니다.
 */
class SE_ANNOTATION(=meta::EditorOnly, =meta::Resource) RealTime final : public detail::TimeTick
{
    friend class TimeManager;
    friend struct TimeResources_Registrar;
};

/**
 * 가상 게임 시간입니다. World별로 독립적으로 관리됩니다.
 * time_scale과 pause 상태에 따라 delta가 조절됩니다.
 */
class SE_ANNOTATION(=meta::EditorOnly, =meta::Resource) GameTime final : public detail::TimeTick
{
    friend class TimeManager;
    friend struct TimeResources_Registrar;

public:
    /** 현재 시간 배율을 반환합니다. (기본값: 1.0) */
    [[nodiscard]] double GetTimeScale() const { return time_scale; }

    /** 일시정지 상태인지 반환합니다. */
    [[nodiscard]] bool IsPaused() const { return paused; }

    /** 시간 배율을 설정합니다. */
    void SetTimeScale(double scale) { time_scale = scale; }

    /** 일시정지 상태를 설정합니다. */
    void SetPaused(bool pause) { paused = pause; }

private:
    SE_ANNOTATION(=meta::Property, =meta::Range(0.1f, 10.0f))
    double time_scale = 1.0;

    SE_ANNOTATION(=meta::Property)
    bool paused = false;
};

/**
 * 고정 시간 스텝입니다. World별로 독립적으로 관리됩니다.
 * 물리 시뮬레이션 등 일정한 간격의 업데이트가 필요한 곳에서 사용합니다.
 * accumulator가 fixed_step 이상이면 FixedUpdatePhase가 실행됩니다.
 */
class SE_ANNOTATION(=meta::EditorOnly, =meta::Resource) FixedTime final : public detail::TimeTick
{
    friend class TimeManager;
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
    SE_ANNOTATION(=meta::Property)
    double fixed_step = 1.0 / 64.0;

    // accumulator는 내부 누적 상태이므로 리플렉션 대상에서 제외합니다.
    double accumulator = 0.0;
};
} // namespace se

SE_DECLARE_REFLECTION(se::RealTime)
SE_DECLARE_REFLECTION(se::GameTime)
SE_DECLARE_REFLECTION(se::FixedTime)
