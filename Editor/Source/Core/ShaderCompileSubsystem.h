#pragma once

#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
/**
 * @todo docs
 */
class SE_ANNOTATION(=meta::Internal) ShaderCompileSubsystem : public se::SubsystemBase, public se::IUpdatable
{
    SE_CLASS(ShaderCompileSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    //~ Begin IUpdatable
    virtual void Update(f64 delta_time) override;
    //~ End IUpdatable
};
} // namespace se::editor
