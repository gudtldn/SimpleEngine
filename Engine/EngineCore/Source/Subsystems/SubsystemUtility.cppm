export module SE.Subsystems.Utility;

import SE.App;
import SE.Interface.ISubsystemBase;
import std;


/**
 * Engine에 등록된 Subsystem을 가져옵니다.
 * @tparam Subsystem 가져올 Subsystem 타입
 * @return Subsystem을 반환, 등록되어 있지 않다면 nullptr
 */
export template <typename Subsystem>
    requires std::derived_from<Subsystem, ISubsystemBase>
Subsystem* GetSubsystemUnchecked()
{
    return se::app::Application::Get().GetEngine().GetSubsystem<Subsystem>();
}

/**
 * Engine에 등록된 Subsystem 여러개를 std::tuple에 담아 가져옵니다.
 * @tparam Subsystems 가져올 Subsystem 타입들
 * @return Subsystem을 tuple에 담아서 반환.
 *         만약 등록되어 있지 않은 Subsystem이 있다면 그 Subsystem은 nullptr
 */
export template <typename... Subsystems>
    requires (std::derived_from<Subsystems, ISubsystemBase> && ...)
std::tuple<Subsystems*...> GetSubsystemsUnchecked()
{
    return { GetSubsystemUnchecked<Subsystems>()... };
}
