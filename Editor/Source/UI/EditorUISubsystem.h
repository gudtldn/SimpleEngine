#pragma once

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Interfaces/ISubsystem.h"
#include "SimpleEngine/Core/Interfaces/IUpdatable.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"
#include "SimpleEngine/Reflection/SubsystemRegistration.h"
#include "SimpleEngine/World/World.h"
#include "UI/Panels/IEditorPanel.h"


namespace se::editor::ui
{
struct EditorUIContext
{
    float delta_time;
    Optional<world::Entity> selected_entity;
};

class EditorUISubsystem
    : public core::ISubsystem<PlatformSubsystem, RenderSubsystem>,
      public core::IUpdatable
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

public:
    template <typename PanelType, typename... Args>
        requires std::derived_from<PanelType, IEditorPanel>
    void RegisterPanel(Args&&... args);

private:
    void SetupDockSpace();
    void DrawMainMenu();

private:
    Array<std::unique_ptr<IEditorPanel>> panels;
    EditorUIContext context{};
};

template <typename PanelType, typename ... Args>
    requires std::derived_from<PanelType, IEditorPanel>
void EditorUISubsystem::RegisterPanel(Args&&... args)
{
    panels.Emplace(std::make_unique<PanelType>(std::forward<Args>(args)...));
}
}
