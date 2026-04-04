#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
// forward declaration
class TimeManager;
struct TimeResources_Registrar;

/**
 * 시간 데이터의 공통 기저 클래스입니다.
 * delta(프레임 경과), elapsed(누적 경과), frame_count를 제공합니다.
 */
class TimeTick
{
    friend class TimeManager;

    // TODO: C++26에서 std::meta::access_context::unchecked()로 접근하면 friend가 필요 없어짐
    friend struct TimeResources_Registrar;

public:
    /** 이번 프레임 경과 시간(초)를 반환합니다. */
    [[nodiscard]] double GetDelta() const { return delta; }

    /** 누적 경과 시간(초)를 반환합니다. */
    [[nodiscard]] double GetElapsed() const { return elapsed; }

    /** 현재까지의 프레임 카운트를 반환합니다. */
    [[nodiscard]] uint64 GetFrameCount() const { return frame_count; }

protected:
    double delta = 0.0;
    double elapsed = 0.0;
    uint64 frame_count = 0;
};
} // namespace se
