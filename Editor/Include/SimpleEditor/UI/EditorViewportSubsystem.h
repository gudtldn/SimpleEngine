#pragma once

#include "SimpleEditor/Camera/EditorCameraState.h"
#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Graphics/View/RenderView.h"

// forward declaration
namespace se
{
class InputSubsystem;
namespace graphics{ class RenderDevice; }
namespace editor{ class ViewportPanel; }
}


namespace se::editor
{
/**
 * Viewport Render Data
 */
struct ViewportRenderInfo
{
    graphics::RID color_texture;
    graphics::RenderView render_view;
    StringName color_target_name;
    StringName depth_target_name;
    bool is_focused = false;
    bool is_hovered = false;
};

/**
 * 에디터 내의 뷰포트(씬 렌더링 창) 상태와 렌더링 리소스를 관리하는 Subsystem
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) EditorViewportSubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(EditorViewportSubsystem, SubsystemBase)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    //~ Begin IUpdatable
    virtual void Update(float delta_time) override;
    //~ End IUpdatable

public:
    /**
     * 뷰포트의 크기를 갱신합니다.
     * @note 크기가 변경되었거나 없을 경우 렌더 타겟을 재생성합니다
     */
    void UpdateViewportSize(const StringName& viewport_id, uint32 new_width, uint32 new_height);

    /** 뷰포트의 ImGui 포커스/호버 상태를 갱신합니다. */
    void UpdateViewportFocus(const StringName& viewport_id, bool focused, bool hovered);

    /** ImGui로 렌더링하기 위한 TextureID(void*)를 반환합니다. */
    [[nodiscard]] void* GetViewportTextureID(const StringName& viewport_id) const;

    /** 현재 관리 중인 모든 활성 뷰포트의 렌더링 정보를 반환합니다. */
    [[nodiscard]] const HashMap<StringName, ViewportRenderInfo>& GetActiveViewportInfo() const { return viewport_data; }

    /** 카메라 조작(우클릭 드래그) 중인 뷰포트가 있는지 확인합니다. */
    [[nodiscard]] bool IsAnyCameraActive() const { return active_camera_viewport != StringName::None; }

    /** 현재 관리 중인 모든 뷰포트 카메라 상태를 반환합니다. */
    [[nodiscard]] const HashMap<StringName, EditorCameraState>& GetViewportCameras() const { return viewport_cameras; }
    [[nodiscard]] HashMap<StringName, EditorCameraState>& GetViewportCameras() { return viewport_cameras; }

    /** 특정 뷰포트의 카메라 상태를 반환합니다. */
    [[nodiscard]] Optional<EditorCameraState&> GetViewportCamera(const StringName& viewport_id)
    {
        return viewport_cameras.Find(viewport_id);
    }

private:
    InputSubsystem* input_subsystem = nullptr;

    graphics::RenderDevice* render_device = nullptr;
    HashMap<StringName, ViewportRenderInfo> viewport_data;
    HashMap<StringName, EditorCameraState> viewport_cameras;

    StringName active_camera_viewport;
    Vector2f last_mouse_pos;
};
} // namespace se::editor
