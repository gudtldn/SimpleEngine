export module SimpleEngine.Subsystems;
export import SimpleEngine.Interfaces.ISubsystem;

import SimpleEngine.App;
import std;


/**
 * Engine에 등록된 Subsystem을 가져옵니다.
 * @tparam Subsystem 가져올 Subsystem 타입
 * @return Subsystem을 반환, 등록되어 있지 않다면 nullptr
 */
export template <typename Subsystem>
    requires std::derived_from<Subsystem, ISubsystem>
Subsystem* GetSubsystem()
{
    return Application::Get().GetEngine()->GetSubsystem<Subsystem>();
}
