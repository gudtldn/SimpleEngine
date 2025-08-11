export module SimpleEngine.Editor.Subsystems.ShaderCompileSubsystem;

import SimpleEngine.Interface.ISubsystem;
import SimpleEngine.Subsystems.PlatformSubsystem;
import std;


export class ShaderCompileSubsystem : public ISubsystem<PlatformSubsystem>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
