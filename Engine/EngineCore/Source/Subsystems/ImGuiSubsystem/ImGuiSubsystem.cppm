export module SimpleEngine.Subsystems.ImGuiSubsystem;

import SimpleEngine.Interfaces.ISubsystem;
import SimpleEngine.Interfaces.IRenderable;
import std;


export class ImGuiSubsystem : public ISubsystem, public IRenderable
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    virtual std::vector<::std::type_index> GetDependencies() const override;
};
