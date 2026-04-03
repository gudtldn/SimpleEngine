#pragma once

#include "SimpleEngine/Core/Time/TimeTick.h"


namespace se
{
/**
 * 실제 글로벌 시간을 나타냅니다.
 * 게임 일시정지나 time scale의 영향을 받지 않습니다.
 */
class RealTime final : public TimeTick
{
    friend class TimeManager;
};
} // namespace se
