#include "SimpleEngine/ECS/SystemChain.h"

#include <ranges>


namespace se
{
void SystemChain::Execute(World& world)
{
    const bool should_execute = std::ranges::all_of(preconditions, [&world](const auto& condition)
    {
        return condition(world);
    });

    if (should_execute)
    {
        for (System& system : systems)
        {
            system.Execute(world);
        }
    }
}
} // namespace se
