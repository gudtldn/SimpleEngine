#include "ECS/WorldSubsystem.h"

#include "Core/Subsystem/SubsystemRegistration.h"
#include "ECS/World.h"


namespace se
{
using namespace ecs;

SE_REGISTER_SUBSYSTEM(WorldSubsystem);

bool WorldSubsystem::Initialize()
{
    world = std::make_unique<World>();
    return true;
}

void WorldSubsystem::Release()
{
    world.reset();
}

void WorldSubsystem::PreUpdate()
{
    world->RunSchedule<schedule::PreUpdate>();
}

void WorldSubsystem::Update(float delta_time)
{
    // TODO: delta_time ECS에서 사용할 수 있도록 수정
    (void)delta_time;

    world->RunSchedule<schedule::Update>();
}

void WorldSubsystem::PostUpdate()
{
    world->RunSchedule<schedule::PostUpdate>();
}
}
