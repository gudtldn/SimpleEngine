module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Subsystems;
import SimpleEngine.Subsystems.PlatformSubsystem;


bool RenderSubsystem::Initialize()
{
    const PlatformSubsystem* platform_subsystem = GetSubsystem<PlatformSubsystem>();
    cached_window = platform_subsystem->GetWindow();

    return true;
}

void RenderSubsystem::Release()
{
}

std::vector<std::type_index> RenderSubsystem::GetDependencies() const
{
    return {
        typeid(PlatformSubsystem)
    };
}
