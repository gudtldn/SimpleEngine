export module SE.Subsystems.WorldSubsystem;

import SE.Interface.ISubsystem;
import SE.Core;
import std;

using namespace se::core::ecs;


/**
 *
 */
export class WorldSubsystem : public ISubsystem<>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    [[nodiscard]] World* GetWorld() const noexcept { return world.get(); }

private:
    // TODO: 나중에 다중 월드로 관리
    std::unique_ptr<World> world;
};
