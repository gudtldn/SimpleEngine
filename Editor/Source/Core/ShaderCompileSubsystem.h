#pragma once
#include "SimpleEngine/Core/Interfaces/ISubsystem.h"
#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Registration/SubsystemRegistration.h"


/**
 * @todo docs
 */
class ShaderCompileSubsystem : public se::core::ISubsystem<PlatformSubsystem>
{
    SE_REGISTER_SUBSYSTEM(ShaderCompileSubsystem)

public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem
};
