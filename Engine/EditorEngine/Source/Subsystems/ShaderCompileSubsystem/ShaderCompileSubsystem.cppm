export module SE.Editor.Subsystems.ShaderCompileSubsystem;

import SE.Interface.ISubsystem;
import SE.Subsystems.PlatformSubsystem;
import std;


export class ShaderCompileSubsystem : public ISubsystem<PlatformSubsystem>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
