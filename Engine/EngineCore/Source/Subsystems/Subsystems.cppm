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

/**
 * Engine에 등록된 Subsystem을 한번에 여러개를 가져옵니다.
 * @tparam Subsystems 가져올 Subsystem 타입들
 * @return Subsystem을 tuple에 담아서 반환.
 *         만약 등록되어 있지 않은 Subsystem이 있다면 그 Subsystem은 nullptr
 */
export template <typename... Subsystems>
    requires (std::derived_from<Subsystems, ISubsystem> && ...)
std::tuple<Subsystems*...> GetSubsystems()
{
    return { Application::Get().GetEngine()->GetSubsystem<Subsystems>()... };
}
