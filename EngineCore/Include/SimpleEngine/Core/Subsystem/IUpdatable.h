#pragma once


namespace se
{
/**
 * 매 프레임 논리적 업데이트가 필요한 객체들이 구현해야 하는 인터페이스
 */
class IUpdatable
{
public:
    virtual ~IUpdatable() = default;

    /** Update 이전에 호출되는 함수입니다. */
    virtual void PreUpdate() {}

    /**
     * 매 프레임 호출되어 상태를 갱신합니다.
     * @param delta_time 이전 프레임과의 시간 간격 (초)
     */
    virtual void Update([[maybe_unused]] float delta_time) {}

    /** Update 이후에 호출되는 함수입니다. */
    virtual void PostUpdate() {}
};
}
