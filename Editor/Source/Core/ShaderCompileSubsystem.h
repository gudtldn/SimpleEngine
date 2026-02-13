#pragma once
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
/**
 * @todo docs
 */
class SE_ANNOTATION(=meta::Internal) ShaderCompileSubsystem : public se::SubsystemBase
{
    SE_CLASS(ShaderCompileSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase
};
}
