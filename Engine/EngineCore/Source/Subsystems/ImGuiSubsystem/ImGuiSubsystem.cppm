export module SimpleEngine.Subsystems.ImGuiSubsystem;

import SimpleEngine.Interfaces.ISubsystem;
import SimpleEngine.Interfaces.IUpdatable;
import SimpleEngine.Subsystems.PlatformSubsystem;
import SimpleEngine.Subsystems.RenderSubsystem;
import std;


export class ImGuiSubsystem : public ISubsystem<PlatformSubsystem, RenderSubsystem>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
