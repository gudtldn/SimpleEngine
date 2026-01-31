#pragma once

#include "Core/EditorSubsystem.h"
#include "UI/Panels/IEditorPanel.h"

#include "SimpleEngine/Core/HAL/PlatformSubsystem.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"
#include "SimpleEngine/Utility/Debug.h"
#include "SimpleEngine/ECS/World.h"


namespace se::editor::ui
{
class EditorUISubsystem : public ISubsystem, public IUpdatable
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

public:
    template <typename PanelType, typename... Args>
        requires std::derived_from<PanelType, IEditorPanel>
    PanelType& RegisterPanel(const StringName& panel_id, Args&&... args);

    [[nodiscard]] Optional<const IEditorPanel&> GetPanel(const StringName& panel_id) const;

private:
    void SetupDockSpace();
    void DrawMainMenu();

private:
    HashMap<StringName, std::unique_ptr<IEditorPanel>> panels; // TODO: Key GUID로 바꿀까
};

template <typename PanelType, typename... Args>
    requires std::derived_from<PanelType, IEditorPanel>
PanelType& EditorUISubsystem::RegisterPanel(const StringName& panel_id, Args&&... args)
{
    SE_ASSERT(!panels.Contains(panel_id), "Panel '{}' is already registered", panel_id);

    auto panel = std::make_unique<PanelType>(std::forward<Args>(args)...);
    PanelType& panel_ref = *panel;

    panels.Emplace(panel_id, std::move(panel));
    return panel_ref;
}
}
