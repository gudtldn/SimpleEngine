#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/UI/IEditorPanel.h"

#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::editor
{
class SE_EDITOR_API SE_ANNOTATION(meta::Internal) EditorUISubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(EditorUISubsystem, SubsystemBase)

public:
    EditorUISubsystem() = default;
    virtual ~EditorUISubsystem() override = default;

    // unique_ptr 멤버로 인해 복사/이동 불가
    EditorUISubsystem(const EditorUISubsystem&) = delete;
    EditorUISubsystem& operator=(const EditorUISubsystem&) = delete;
    EditorUISubsystem(EditorUISubsystem&&) = delete;
    EditorUISubsystem& operator=(EditorUISubsystem&&) = delete;

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void PreUpdate() override;
    virtual void Update(double delta_time) override;
    virtual void PostUpdate() override;
    //~ End IUpdatable

public:
    template <typename PanelType, typename... Args>
        requires std::derived_from<PanelType, IEditorPanel>
    PanelType& RegisterPanel(const StringName& panel_id, Args&&... args);

    [[nodiscard]] Optional<const IEditorPanel&> GetPanel(const StringName& panel_id) const;

    /** 등록된 패널 중 하나라도 ImGui 포커스 상태인지 확인합니다. */
    [[nodiscard]] bool IsAnyPanelFocused() const;

private:
    void SetupDockSpace();
    void DrawMainMenu();

private:
    HashMap<StringName, std::unique_ptr<IEditorPanel>> panels; // TODO: Key GUID로 바꿀까
    DelegateHandle sdl_event_handle;
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
} // namespace se::editor
