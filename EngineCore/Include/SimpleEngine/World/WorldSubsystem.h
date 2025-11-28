#pragma once
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/World/World.h"


/**
 *
 */
class SE_CORE_API WorldSubsystem : public se::core::ISubsystem, public se::core::IUpdatable
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

    [[nodiscard]] se::world::World* GetWorld() const noexcept { return world.get(); }

private:
    // TODO: 나중에 다중 월드로 관리
    std::unique_ptr<se::world::World> world;
};
