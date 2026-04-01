#include "SimpleEngine/ECS/System.h"

#include <ranges>


namespace se
{
void System::Execute(World& world)
{
    const bool should_execute = std::ranges::all_of(preconditions, [&world](const auto& condition)
    {
        return condition(world);
    });

    if (should_execute)
    {
        system(world);
    }
}
} // namespace se
