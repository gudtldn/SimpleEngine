#include "SimpleEngine/ECS/Schedule.h"


namespace se
{
void Schedule::Execute(World& world)
{
    for (Function<void(World&)>& executable : executables)
    {
        executable(world);
    }
}
} // namespace se
