export module SE.Interface.ISubsystem;

import SE.Core;
import SE.Traits;
import SE.Subsystems.Utility;
import SE.Interface.ISubsystemBase;
import std;


/** Subsystem이 Dependencies에 포함되어 있는지 검사합니다. */
template <typename Subsystem, typename... Dependencies>
concept IsDependency = se::traits::type_traits::IsAnyOf<std::remove_cv_t<Subsystem>, Dependencies...>;

export template <typename... Dependencies>
    requires (std::derived_from<Dependencies, ISubsystemBase> && ...)
class ISubsystem : public ISubsystemBase
{
public:
    /** 이 Subsystem이 의존하는 모든 타입의 type_index를 반환합니다. */
    virtual se::vector<std::type_index> GetDependencies() const final override
    {
        return { typeid(Dependencies)... };
    }

    /**
     * Engine에 등록된 Subsystem을 가져옵니다.
     * @tparam Subsystem 가져올 Subsystem 타입
     * @return Subsystem 포인터, 등록되어 있지 않으면 nullptr
     * @note Subsystem이 Dependencies에 포함되어 있지 않으면 컴파일 에러가 발생합니다.
     */
    template <typename Subsystem>
    requires
        std::derived_from<Subsystem, ISubsystemBase>
        && IsDependency<Subsystem, Dependencies...>
    Subsystem* GetSubsystem() const
    {
        return GetSubsystemUnchecked<Subsystem>();
    }

    /**
     * Engine에 등록된 Subsystem 여러개를 std::tuple에 담아 가져옵니다.
     * @tparam Subsystems 가져올 Subsystem 타입들
     * @return Subsystem 포인터들을 담은 tuple, 등록되어 있지 않으면 nullptr
     * @note Subsystem이 Dependencies에 포함되어 있지 않으면 컴파일 에러가 발생합니다.
     */
    template <typename... Subsystems>
    requires
        (std::derived_from<Subsystems, ISubsystemBase> && ...)
        && (IsDependency<Subsystems, Dependencies...>, ...)
    std::tuple<Subsystems*...> GetSubsystems() const
    {
        return GetSubsystemsUnchecked<Subsystems...>();
    }
};
