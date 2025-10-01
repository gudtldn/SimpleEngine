export module SE.Editor.Subsystems.EditorUISubsystem;

import SE.Interface.ISubsystem;
import SE.Interface.IUpdatable;
import SE.Subsystems.PlatformSubsystem;
import SE.Subsystems.RenderSubsystem;
import std;


export class EditorUISubsystem : public ISubsystem<PlatformSubsystem, RenderSubsystem>, public IUpdatable
{
public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem

    //~ Begin IUpdatable
    virtual void PreUpdate() override;
    virtual void Update(float delta_time) override;
    virtual void PostUpdate() override;
    //~ End IUpdatable
};
