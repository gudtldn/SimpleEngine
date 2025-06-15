export module SimpleEngine.Interfaces.ISubsystem;
import std;


/**
 * Engine에서 사용되는 Subsystem의 기본 구조를 정의하는 인터페이스 클래스
 */
export class ISubsystem
{
public:
    virtual ~ISubsystem() = default;

    [[nodiscard]] virtual bool Initialize() = 0;
    virtual void Release() = 0;

    virtual std::vector<std::type_index> GetDependencies() const
    {
        return {};
    }
};
