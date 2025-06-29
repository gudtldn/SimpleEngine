export module SimpleEngine.Subsystems.ImGuiSubsystem;

import SimpleEngine.Interfaces.ISubsystem;
import SimpleEngine.Interfaces.IUpdatable;
import SimpleEngine.Interfaces.IRenderable;
import std;


export class ImGuiSubsystem : public ISubsystem, public IUpdatable, public IRenderable
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    virtual void Update(float delta_time) override;

    virtual std::vector<::std::type_index> GetDependencies() const override;
};
