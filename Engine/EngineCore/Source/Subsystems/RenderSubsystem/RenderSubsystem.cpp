module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Subsystems.PlatformSubsystem;


bool RenderSubsystem::Initialize()
{
    // TODO: Implements this
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
