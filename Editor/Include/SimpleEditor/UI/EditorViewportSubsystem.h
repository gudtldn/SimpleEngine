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
/** 기즈모 조작 모드 */
enum class EGizmoMode : uint8
{
    Translate,
    Rotate,
    Scale,
};

/** 기즈모 좌표계 모드 */
enum class ECoordinateSpace : uint8
{
    World,
    Local,
};

/** 뷰포트 뷰 모드 */
enum class EViewMode : uint8
{
    // 원근 뷰
    Perspective,

    // 직교 뷰
    Top,
    Bottom,
    Front,
    Back,
    Right,
    Left,
};

/**
 * 뷰포트 하나의 모든 상태를 담는 구조체
 */
struct ViewportState
{
public:
    // 렌더 리소스
    graphics::RID color_texture;
    graphics::RenderView render_view;
    StringName color_target_name;
    StringName depth_target_name;
    bool is_focused = false;
    bool is_hovered = false;

    // 카메라
    EditorCameraState persp_camera;               // Perspective 전용
    EditorCameraState ortho_camera;               // Orthographic 전용
    EViewMode view_mode = EViewMode::Perspective; // 현재 활성 뷰 모드

    // 인터랙션
    EGizmoMode gizmo_mode = EGizmoMode::Translate;
    ECoordinateSpace coordinate_space = ECoordinateSpace::World;

public:
    /** 현재 활성화된 ViewMode에 대한 카메라를 가져옵니다. */
    [[nodiscard]] FORCE_INLINE EditorCameraState& GetActiveCamera() { return (view_mode == EViewMode::Perspective) ? persp_camera : ortho_camera; }
    [[nodiscard]] FORCE_INLINE const EditorCameraState& GetActiveCamera() const { return (view_mode == EViewMode::Perspective) ? persp_camera : ortho_camera; }

    [[nodiscard]] FORCE_INLINE bool IsPerspectiveView() const { return view_mode == EViewMode::Perspective; }
    [[nodiscard]] FORCE_INLINE bool IsOrthographicView() const { return view_mode != EViewMode::Perspective; }
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
     * @note 크기가 변경되었거나 없을 경우 렌더 타겟을 재생성합니다.
     */
    void UpdateViewportSize(const StringName& viewport_id, uint32 new_width, uint32 new_height);

    /** 뷰포트의 ImGui 포커스/호버 상태를 갱신합니다. */
    void UpdateViewportFocus(const StringName& viewport_id, bool focused, bool hovered);

    /** 뷰포트의 RenderingMode를 설정합니다. */
    void SetViewportRenderingMode(const StringName& viewport_id, graphics::ERenderingMode mode);

    /** 뷰포트의 ShowFlags를 설정합니다. */
    void SetViewportShowFlags(const StringName& viewport_id, graphics::ShowFlags flags);

    /** 뷰포트의 기즈모 모드를 설정합니다. */
    void SetViewportGizmoMode(const StringName& viewport_id, EGizmoMode mode);

    /** 특정 뷰포트의 기즈모 모드를 반환합니다. */
    [[nodiscard]] EGizmoMode GetViewportGizmoMode(const StringName& viewport_id) const;

    /** 뷰포트의 기즈모 좌표계를 설정합니다. */
    void SetViewportCoordinateSpace(const StringName& viewport_id, ECoordinateSpace space);

    /** 특정 뷰포트의 기즈모 좌표계를 반환합니다. */
    [[nodiscard]] ECoordinateSpace GetViewportCoordinateSpace(const StringName& viewport_id) const;

    /** 뷰포트의 뷰 모드를 설정합니다. */
    void SetViewportViewMode(const StringName& viewport_id, EViewMode mode);

    /** 특정 뷰포트의 뷰 모드를 반환합니다. */
    [[nodiscard]] EViewMode GetViewportViewMode(const StringName& viewport_id) const;

public:
    /** 현재 관리 중인 모든 뷰포트의 상태를 반환합니다. */
    [[nodiscard]] const HashMap<StringName, ViewportState>& GetViewports() const { return viewports; }

    /** 특정 뷰포트의 상태를 반환합니다. */
    [[nodiscard]] Optional<const ViewportState&> GetViewportInfo(const StringName& viewport_id) const { return viewports.Find(viewport_id); }

    /** ImGui로 렌더링하기 위한 TextureID(void*)를 반환합니다. */
    [[nodiscard]] void* GetViewportTextureID(const StringName& viewport_id) const;

    /**
     * 현재 포커스된 뷰포트의 ID를 반환합니다.
     * @return 현재 포커스된 뷰포트의 ID, 없으면 StringName::None
     */
    [[nodiscard]] StringName GetFocusedViewportId() const { return focused_viewport; }

    /** 현재 포커스된 뷰포트의 상태를 반환합니다. */
    [[nodiscard]] Optional<const ViewportState&> GetFocusedViewportInfo() const { return viewports.Find(focused_viewport); }

    /** 카메라 조작(우클릭 드래그) 중인 뷰포트가 있는지 확인합니다. */
    [[nodiscard]] bool IsAnyCameraActive() const { return active_camera_viewport != StringName::None; }

    /** 특정 뷰포트의 카메라 상태를 반환합니다. */
    [[nodiscard]] Optional<const EditorCameraState&> GetViewportCamera(const StringName& viewport_id) const;
    [[nodiscard]] Optional<EditorCameraState&> GetViewportCamera(const StringName& viewport_id);

private:
    InputSubsystem* input_subsystem = nullptr;
    graphics::RenderDevice* render_device = nullptr;

    HashMap<StringName, ViewportState> viewports;

    StringName focused_viewport;
    StringName active_camera_viewport;
    Vector2f last_mouse_pos;
};
} // namespace se::editor
