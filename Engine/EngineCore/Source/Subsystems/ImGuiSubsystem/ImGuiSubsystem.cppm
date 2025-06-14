export module SimpleEngine.Subsystems.ImGuiSubsystem;

import SimpleEngine.Core.ISubsystem;
import std;


export class ImGuiSubsystem : public ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    virtual std::vector<::std::type_index> GetDependencies() const override;
};
