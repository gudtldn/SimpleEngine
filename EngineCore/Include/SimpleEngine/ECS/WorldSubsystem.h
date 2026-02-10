#pragma once
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/ECS/World.h"


namespace se
{
/**
 *
 */
class SE_CORE_API WorldSubsystem : public se::SubsystemBase, public se::IUpdatable
{
    SE_CLASS(WorldSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void PreUpdate() override;
    virtual void Update(float delta_time) override;
    virtual void PostUpdate() override;
    //~ End IUpdatable

    [[nodiscard]] se::ecs::World* GetWorld() const noexcept { return world.get(); }

private:
    // TODO: 나중에 다중 월드로 관리
    std::unique_ptr<se::ecs::World> world;
};
}
