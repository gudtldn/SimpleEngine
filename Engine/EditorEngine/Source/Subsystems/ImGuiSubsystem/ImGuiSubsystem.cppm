export module SE.Editor.Subsystems.ImGuiSubsystem;

import SE.Interface.ISubsystem;
import SE.Interface.IUpdatable;
import SE.Subsystems.PlatformSubsystem;
import SE.Subsystems.RenderSubsystem;
import std;


export class ImGuiSubsystem : public ISubsystem<PlatformSubsystem, RenderSubsystem>, public IUpdatable
{
public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

    //~ Begin IUpdatable
    virtual void Update(float delta_time) override;
    //~ End IUpdatable
};
