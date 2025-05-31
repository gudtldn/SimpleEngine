export module SimpleEngine.Subsystems.ConfigSubsystem;

import SimpleEngine.Core.ISubSystem;


export class ConfigSubsystem : public ISubSystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
};
