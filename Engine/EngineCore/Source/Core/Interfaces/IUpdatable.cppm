export module SimpleEngine.Interfaces.IUpdatable;


/**
 * 매 프레임 논리적 업데이트가 필요한 객체들이 구현해야 하는 인터페이스
 */
export class IUpdatable
{
public:
    virtual ~IUpdatable() = default;

    /**
     * 매 프레임 호출되어 상태를 갱신합니다.
     * @param delta_time 이전 프레임과의 시간 간격 (초)
     */
    virtual void Update(float delta_time) = 0;
};
