module SimpleEngine.Subsystems.ImGuiSubsystem;

import SimpleEngine.Subsystems.PlatformSubsystem;
import SimpleEngine.Subsystems.RenderSubsystem;


bool ImGuiSubsystem::Initialize()
{
    // TODO: Implements this
    return true;
}

void ImGuiSubsystem::Release()
{
}

std::vector<std::type_index> ImGuiSubsystem::GetDependencies() const
{
    return {
        typeid(PlatformSubsystem),
        typeid(RenderSubsystem)
    };
}
