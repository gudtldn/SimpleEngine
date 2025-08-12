module SimpleEngine.Subsystems.WorldSubsystem;


bool WorldSubsystem::Initialize()
{
    world = std::make_unique<World>();
    return true;
}

void WorldSubsystem::Release()
{
    world.reset();
}
