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
    PanelType& RegisterPanel(Args&&... args);

    template <typename PanelType>
    Optional<const PanelType&> GetPanel() const;

private:
    void SetupDockSpace();
    void DrawMainMenu();

private:
    HashMap<refl::TypeId, std::unique_ptr<IEditorPanel>> panels;
    EditorUIContext context{};
};

template <typename PanelType, typename ... Args>
    requires std::derived_from<PanelType, IEditorPanel>
PanelType& EditorUISubsystem::RegisterPanel(Args&&... args)
{
    auto panel = std::make_unique<PanelType>(std::forward<Args>(args)...);
    PanelType* raw_ptr = panel.get();

    panels.Emplace(refl::TypeId::Get<PanelType>(), std::move(panel));
    return *raw_ptr;
}

template <typename PanelType>
Optional<const PanelType&> EditorUISubsystem::GetPanel() const
{
    return panels.Find(refl::TypeId::Get<PanelType>())
        .AndThen([](const auto& panel_ptr) -> Optional<const PanelType&>
        {
            return static_cast<const PanelType&>(*panel_ptr);
        });
}
}
