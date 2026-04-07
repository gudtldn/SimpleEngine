#include "SimpleEngine/ECS/CommandBuffer.h"


namespace se
{
void CommandBuffer::Flush(World& world)
{
    for (const auto& cmd : commands)
    {
        cmd(world);
    }
    commands.Clear();
}
} // namespace se
