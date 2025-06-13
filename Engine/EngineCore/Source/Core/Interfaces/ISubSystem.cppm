export module SimpleEngine.Core.ISubSystem;

import SimpleEngine.Platform.Types;
import SimpleEngine.Utils;
import std;


/**
 * Engine에서 사용되는 SubSystem의 기본 구조를 정의하는 인터페이스 클래스
 */
export class ISubSystem
{
public:
    virtual ~ISubSystem() = default;

    [[nodiscard]] virtual bool Initialize() = 0;
    virtual void Release() = 0;

    virtual void Tick([[maybe_unused]] float delta_time)
    {
    }

    virtual std::vector<std::type_index> GetDependencies() const
    {
        return {};
    }
};
