export module SimpleEngine.Interfaces.ISubsystemBase;

import std;


/**
 * Engine에서 사용되는 Subsystem의 기본 구조를 정의하는 인터페이스 클래스
 * @note Subsystem구현은 ISubsystem을 사용해 주세요!
 */
export class ISubsystemBase
{
public:
    virtual ~ISubsystemBase() = default;

    [[nodiscard]] virtual bool Initialize() = 0;
    virtual void Release() = 0;

    virtual std::vector<std::type_index> GetDependencies() const = 0;
};
