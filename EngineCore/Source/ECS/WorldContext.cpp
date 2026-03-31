#include "SimpleEngine/ECS/WorldContext.h"
#include "SimpleEngine/ECS/World.h"


namespace se
{
WorldContext::WorldContext()
    : world(std::make_unique<World>())
{
}

WorldContext::~WorldContext() = default;
} // namespace se
