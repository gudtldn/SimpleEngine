module SimpleEngine.Subsystems.WorldSubsystem;


bool WorldSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing World subsystem...");


    ConsoleLog(ELogLevel::Info, u8"World subsystem initialized");
    return true;
}

void WorldSubsystem::Release()
{
}
