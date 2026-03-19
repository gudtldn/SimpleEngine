#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/RenderSubsystem.h"
#include "SimpleEngine/Graphics/View/RenderView.h"

// forward declaration
namespace se
{
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
};

/**
 * 에디터 내의 뷰포트(씬 렌더링 창) 상태와 렌더링 리소스를 관리하는 Subsystem
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) EditorViewportSubsystem : public SubsystemBase
{
    SE_CLASS(EditorViewportSubsystem, SubsystemBase)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

public:
    /**
     * 뷰포트의 크기를 갱신합니다.
     * @note 크기가 변경되었거나 없을 경우 렌더 타겟을 재생성합니다
     */
    void UpdateViewportSize(const StringName& viewport_id, uint32 new_width, uint32 new_height);

    /** ImGui로 렌더링하기 위한 TextureID(void*)를 반환합니다. */
    [[nodiscard]] void* GetViewportTextureID(const StringName& viewport_id) const;

    /** 현재 관리 중인 모든 활성 뷰포트의 렌더링 정보를 반환합니다. */
    [[nodiscard]] const HashMap<StringName, ViewportRenderInfo>& GetActiveViewportInfo() const { return viewport_data; }

private:
    graphics::RenderDevice* render_device = nullptr;
    HashMap<StringName, ViewportRenderInfo> viewport_data;
};
} // namespace se::editor
