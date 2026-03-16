#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <chrono>


namespace se
{
/**
 * RAII 기반 경과 시간 측정 유틸리티
 *
 * 생성 시점부터의 경과 시간을 밀리초(double) 또는 초(double)로 반환합니다.
 * 콜백이나 로깅을 내장하지 않으므로, 호출부에서 자유롭게 출력 형태를 결정할 수 있습니다.
 *
 * @code
 *   ScopedTimer timer;
 *   DoWork();
 *   ConsoleLog(ELogLevel::Info, "Elapsed: {:.1f}ms", timer.ElapsedMs());
 * @endcode
 */
class ScopedTimer
{
    using Clock = std::chrono::steady_clock;

public:
    ScopedTimer()
        : start(Clock::now())
    {
    }

    /** 경과 시간을 밀리초(ms)로 반환합니다. */
    [[nodiscard]] double ElapsedMs() const
    {
        const auto elapsed = Clock::now() - start;
        return std::chrono::duration<double, std::milli>(elapsed).count();
    }

    /** 경과 시간을 초(s)로 반환합니다. */
    [[nodiscard]] double ElapsedSec() const
    {
        const auto elapsed = Clock::now() - start;
        return std::chrono::duration<double>(elapsed).count();
    }

    /** 타이머를 리셋합니다. */
    void Reset()
    {
        start = Clock::now();
    }

private:
    Clock::time_point start;
};
} // namespace se