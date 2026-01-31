#pragma once
#include <concepts>
#include <tuple>

#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
/**
 * Engine에 등록된 Subsystem을 가져옵니다.
 * @tparam Subsystem 가져올 Subsystem 타입
 * @return Subsystem을 반환, 등록되어 있지 않다면 nullptr
 */
template <typename Subsystem>
    requires std::derived_from<Subsystem, ISubsystem>
Subsystem* GetSubsystem()
{
    return Engine::Get().GetSubsystem<Subsystem>();
}

/**
 * Engine에 등록된 Subsystem을 가져옵니다.
 * Subsystem이 등록되지 않은 경우에는 애플리케이션이 정지되며 로그가 출력됩니다.
 *
 * @tparam Subsystem 가져올 Subsystem 타입
 * @return 등록된 Subsystem의 참조를 반환
 */
template <typename Subsystem>
    requires std::derived_from<Subsystem, ISubsystem>
Subsystem& GetSubsystemChecked()
{
    Subsystem* subsystem = GetSubsystem<Subsystem>();
    SE_ASSERT(subsystem, "Subsystem {} is not registered.", refl::GetFullTypeName<Subsystem>());
    return *subsystem;
}

/**
 * Engine에 등록된 Subsystem 여러개를 std::tuple에 담아 가져옵니다.
 * @tparam Subsystems 가져올 Subsystem 타입들
 * @return Subsystem을 tuple에 담아서 반환
 *         만약 등록되어 있지 않은 Subsystem이 있다면 그 Subsystem은 nullptr
 */
template <typename... Subsystems>
    requires (std::derived_from<Subsystems, ISubsystem> && ...)
std::tuple<Subsystems*...> GetSubsystems()
{
    return { GetSubsystem<Subsystems>()... };
}

/**
 * Engine에 등록된 Subsystem 여러 개를 std::tuple에 담아 가져옵니다.
 * Subsystem이 등록되지 않은 경우 애플리케이션이 정지되며 로그가 출력됩니다.
 *
 * @tparam Subsystems 가져올 Subsystem 타입들
 * @return 등록된 Subsystem의 참조를 std::tuple에 담아 반환
 */
template <typename... Subsystems>
    requires (std::derived_from<Subsystems, ISubsystem> && ...)
std::tuple<Subsystems&...> GetSubsystemsChecked()
{
    return { GetSubsystemChecked<Subsystems>()... };
}
}
