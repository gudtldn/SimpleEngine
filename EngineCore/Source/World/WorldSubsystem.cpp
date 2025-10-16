#include "World/WorldSubsystem.h"
#include "World/World.h"

using namespace se::world;


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
    world->RunSchedule<schedules::PreUpdate>();
}

void WorldSubsystem::Update(float delta_time)
{
    // TODO: delta_time ECS에서 사용할 수 있도록 수정
    (void)delta_time;

    world->RunSchedule<schedules::Update>();
}

void WorldSubsystem::PostUpdate()
{
    world->RunSchedule<schedules::PostUpdate>();
}
