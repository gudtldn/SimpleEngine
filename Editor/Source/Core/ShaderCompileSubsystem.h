#pragma once
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"


namespace se::editor
{
/**
 * @todo docs
 */
class ShaderCompileSubsystem : public se::core::ISubsystem
{
public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem
};
}
