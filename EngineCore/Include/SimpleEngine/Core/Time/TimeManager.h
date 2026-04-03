#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Time/FixedTime.h"
#include "SimpleEngine/Core/Time/GameTime.h"
#include "SimpleEngine/Core/Time/RealTime.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se
{
/**
 * 엔진 전체의 시간 데이터를 중앙 관리하는 매니저 클래스
 * Engine이 소유하며, 매 프레임 Update()가 호출됩니다.
 *
 * - RealTime: 글로벌 시간 (Application의 SDL 타이머 기반)
 * - GameTime: World별 게임 시간 (scale/pause 지원)
 * - FixedTime: World별 고정 시간 (accumulator 기반)
 */
class SE_CORE_API TimeManager
{
public:
    /** 매 프레임 Application으로부터 받은 raw delta로 모든 시간을 갱신합니다. */
    void Update(float raw_delta);

    /** 글로벌 RealTime을 반환합니다. */
    [[nodiscard]] const RealTime& GetRealTime() const { return real_time; }

    /** World를 시간 관리에 등록합니다. */
    void RegisterWorld(const StringName& name);

    /** World를 시간 관리에서 제거합니다. */
    void UnregisterWorld(const StringName& name);

    /** 해당 World의 GameTime을 반환합니다. */
    [[nodiscard]] GameTime& GetGameTime(const StringName& world_name);
    [[nodiscard]] const GameTime& GetGameTime(const StringName& world_name) const;

    /** 해당 World의 FixedTime을 반환합니다. */
    [[nodiscard]] FixedTime& GetFixedTime(const StringName& world_name);
    [[nodiscard]] const FixedTime& GetFixedTime(const StringName& world_name) const;

private:
    struct WorldTimeState
    {
        GameTime game_time;
        FixedTime fixed_time;
    };

    RealTime real_time;
    HashMap<StringName, WorldTimeState> world_times;
};
} // namespace se
