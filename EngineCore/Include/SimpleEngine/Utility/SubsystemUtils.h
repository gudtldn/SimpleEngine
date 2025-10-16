#pragma once
#include <concepts>
#include <tuple>

#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/Interfaces/ISubsystemBase.h"


namespace se::utility
{
/**
 * Engine에 등록된 Subsystem을 가져옵니다.
 * @tparam Subsystem 가져올 Subsystem 타입
 * @return Subsystem을 반환, 등록되어 있지 않다면 nullptr
 */
template <typename Subsystem>
    requires std::derived_from<Subsystem, core::ISubsystemBase>
Subsystem* GetSubsystemUnchecked()
{
    return app::Application::Get().GetEngine().GetSubsystem<Subsystem>();
}

/**
 * Engine에 등록된 Subsystem 여러개를 std::tuple에 담아 가져옵니다.
 * @tparam Subsystems 가져올 Subsystem 타입들
 * @return Subsystem을 tuple에 담아서 반환.
 *         만약 등록되어 있지 않은 Subsystem이 있다면 그 Subsystem은 nullptr
 */
template <typename... Subsystems>
    requires (std::derived_from<Subsystems, core::ISubsystemBase> && ...)
std::tuple<Subsystems*...> GetSubsystemsUnchecked()
{
    return { GetSubsystemUnchecked<Subsystems>()... };
}
}
