export module SimpleEngine.Interfaces.ISubsystem;

import SimpleEngine.Core.TypeTraits;
import SimpleEngine.Subsystems.Utils;
import SimpleEngine.Interfaces.ISubsystemBase;
import std;


template <typename Subsystem, typename... Dependencies>
concept IsDependency = TIsAnyOf<Subsystem, Dependencies...>;

export template <typename... Dependencies>
class ISubsystem : public ISubsystemBase
{
public:
    virtual std::vector<std::type_index> GetDependencies() const final override
    {
        return { typeid(Dependencies)... };
    }

    template <typename Subsystem>
    requires
        std::derived_from<Subsystem, ISubsystemBase>
        && IsDependency<Subsystem, Dependencies...> // Subsystem is not in Dependencies!
    Subsystem* GetSubsystem()
    {
        return GetSubsystemUnchecked<Subsystem>();
    }

    template <typename... Subsystems>
    requires
        (std::derived_from<Subsystems, ISubsystemBase> && ...)
        && (IsDependency<Subsystems, Dependencies...>, ...) // Subsystem is not in Dependencies!
    std::tuple<Subsystems*...> GetMutableSubsystems()
    {
        return GetMutableSubsystemsUnchecked<Subsystems...>();
    }

    template <typename... Subsystems>
    requires
        (std::derived_from<Subsystems, ISubsystemBase> && ...)
        && (IsDependency<Subsystems, Dependencies...>, ...) // Subsystem is not in Dependencies!
    std::tuple<const Subsystems*...> GetSubsystems()
    {
        return GetMutableSubsystems<Subsystems...>();
    }
};
