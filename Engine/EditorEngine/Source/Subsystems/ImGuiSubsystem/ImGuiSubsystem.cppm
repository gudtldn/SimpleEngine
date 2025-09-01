export module SE.Editor.Subsystems.ImGuiSubsystem;

import SE.Interface.ISubsystem;
import SE.Subsystems.PlatformSubsystem;
import SE.Subsystems.RenderSubsystem;
import std;


export class ImGuiSubsystem : public ISubsystem<PlatformSubsystem, RenderSubsystem>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
