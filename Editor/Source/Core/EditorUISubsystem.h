#pragma once
#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Interfaces/ISubsystem.h"
#include "SimpleEngine/Core/Interfaces/IUpdatable.h"
#include "SimpleEngine/Reflection/SubsystemRegistration.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"


class EditorUISubsystem
    : public se::core::ISubsystem<PlatformSubsystem, RenderSubsystem>,
      public se::core::IUpdatable
{
    SE_REGISTER_SUBSYSTEM(EditorUISubsystem)

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
};
