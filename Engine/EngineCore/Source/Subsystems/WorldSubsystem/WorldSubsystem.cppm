export module SE.Subsystems.WorldSubsystem;

import SE.Interface.ISubsystem;
import SE.Interface.IUpdatable;
import SE.Core;
import std;

using namespace se::core::ecs;


/**
 *
 */
export class WorldSubsystem : public ISubsystem<>, public IUpdatable
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

    [[nodiscard]] World* GetWorld() const noexcept { return world.get(); }

private:
    // TODO: 나중에 다중 월드로 관리
    std::unique_ptr<World> world;
};
