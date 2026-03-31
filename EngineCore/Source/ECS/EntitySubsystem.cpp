#include "SimpleEngine/ECS/EntitySubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"
#include "SimpleEngine/ECS/World.h"


namespace se
{
SE_REGISTER_SUBSYSTEM(EntitySubsystem);

SE_BEGIN_REFLECT(EntitySubsystem, meta::Internal)
    SE_REFLECT_INTERFACE(IUpdatable)
SE_END_REFLECT(EntitySubsystem)

bool EntitySubsystem::Initialize()
{
    world = std::make_unique<World>();
    return true;
}

void EntitySubsystem::Release()
{
    world.reset();
}

void EntitySubsystem::PreUpdate()
{
    world->RunPhase<PreUpdatePhase>();
}

void EntitySubsystem::Update(float delta_time)
{
    // TODO: delta_time ECS에서 사용할 수 있도록 수정
    (void)delta_time;

    world->RunPhase<UpdatePhase>();
}

void EntitySubsystem::PostUpdate()
{
    world->RunPhase<PostUpdatePhase>();
}
} // namespace se
