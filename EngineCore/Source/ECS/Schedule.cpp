#include "SimpleEngine/ECS/Schedule.h"

#include "SimpleEngine/ECS/CommandBuffer.h"
#include "SimpleEngine/ECS/World.h"


namespace se
{
void Schedule::Execute(World& world)
{
    CommandBuffer buffer;
    world.SetActiveCommandBuffer(&buffer);

    for (const auto& executable : executables)
    {
        executable(world);
        buffer.Flush(world);
    }

    world.SetActiveCommandBuffer(nullptr);
}
} // namespace se
