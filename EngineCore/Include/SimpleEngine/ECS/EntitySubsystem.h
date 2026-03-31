#pragma once
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/ECS/World.h"


namespace se
{
/**
 * ECS World의 생명주기를 관리하고 엔진 업데이트 루프와 연결하는 Subsystem
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) EntitySubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(EntitySubsystem, SubsystemBase)

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

    [[nodiscard]] World* GetWorld() const noexcept { return world.get(); }

private:
    // TODO: 나중에 다중 월드로 관리
    std::unique_ptr<World> world;
};
} // namespace se
