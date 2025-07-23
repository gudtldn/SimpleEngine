export module SimpleEngine.Interface.ISubsystem;

import SimpleEngine.Core;
import SimpleEngine.TypeTraits;
import SimpleEngine.Subsystems.Utility;
import SimpleEngine.Interface.ISubsystemBase;
import std;


/** Subsystem이 Dependencies에 포함되어 있는지 검사합니다. */
template <typename Subsystem, typename... Dependencies>
concept IsDependency = se::type_traits::TIsAnyOf<Subsystem, Dependencies...>;

export template <typename... Dependencies>
class ISubsystem : public ISubsystemBase
{
public:
    /** 이 Subsystem이 의존하는 모든 타입의 type_index를 반환합니다. */
    virtual std::vector<std::type_index> GetDependencies() const final override
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
    Subsystem* GetSubsystem()
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
    std::tuple<Subsystems*...> GetMutableSubsystems()
    {
        return GetMutableSubsystemsUnchecked<Subsystems...>();
    }

    /**
     * Engine에 등록된 Subsystem 여러개를 const로 std::tuple에 담아 가져옵니다.
     * @tparam Subsystems 가져올 Subsystem 타입들
     * @return const Subsystem 포인터들을 담은 tuple, 등록되어 있지 않으면 nullptr
     * @note Subsystem이 Dependencies에 포함되어 있지 않으면 컴파일 에러가 발생합니다.
     */
    template <typename... Subsystems>
    requires
        (std::derived_from<Subsystems, ISubsystemBase> && ...)
        && (IsDependency<Subsystems, Dependencies...>, ...)
    std::tuple<const Subsystems*...> GetSubsystems()
    {
        return GetMutableSubsystems<Subsystems...>();
    }
};
