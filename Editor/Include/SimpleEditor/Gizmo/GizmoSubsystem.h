#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEditor/Gizmo/GizmoInteraction.h"
#include "SimpleEditor/Gizmo/GizmoRenderer.h"
#include "SimpleEditor/Gizmo/GizmoTypes.h"

#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
{
/**
 * 기즈모 렌더링 리소스를 소유하고 관리하는 Subsystem
 * GizmoDrawList(GPU 버퍼) + GizmoRenderer(형상 조립)의 생명주기를 관리합니다.
 * GPU Color Picking용 텍스처와 Readback 버퍼도 소유합니다.
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) GizmoSubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(GizmoSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void Update(double delta_time) override;
    //~ End IUpdatable

public:
    /**
     * 이전 프레임 데이터를 초기화하고, 선택된 엔티티에 대한 기즈모를 그립니다.
     * 매 프레임 GPU 업로드 전에 호출해야 합니다.
     */
    void DrawGizmos();

    /**
     * GPU Readback으로 pick 텍스처에서 ID를 읽어 hovered_axis를 갱신합니다.
     * RenderFrame() 완료 후 호출해야 합니다.
     */
    void PerformPick();

    /** GPU Color Picking용 1x1 R32_UINT 텍스처 (RenderGraph ImportTexture용) */
    [[nodiscard]] SDL_GPUTexture* GetPickTexture() const;

    /** 매 프레임 정점을 수집/업로드하는 드로우 리스트 접근자 */
    [[nodiscard]] GizmoDrawList& GetDrawList() { return *draw_list; }
    [[nodiscard]] const GizmoDrawList& GetDrawList() const { return *draw_list; }

    /** 기즈모 형상 조립(Translate/Rotate/Scale) 렌더러 접근자 */
    [[nodiscard]] GizmoRenderer& GetRenderer() { return renderer; }
    [[nodiscard]] const GizmoRenderer& GetRenderer() const { return renderer; }

    /** 기즈모 드래그 인터랙션 접근자 */
    [[nodiscard]] GizmoInteraction& GetInteraction() { return interaction; }
    [[nodiscard]] const GizmoInteraction& GetInteraction() const { return interaction; }

    /** 현재 마우스가 hover 중인 기즈모 축 */
    [[nodiscard]] EGizmoAxis GetHoveredAxis() const { return hovered_axis; }

    /** 현재 드래그 중인지 확인합니다. */
    [[nodiscard]] bool IsDragging() const { return interaction.IsDragging(); }

private:
    /**
     * 마우스 입력을 확인하여 드래그 시작/업데이트/종료를 처리하고,
     * 결과 delta를 선택된 엔티티의 TransformComponent에 적용합니다.
     */
    void HandleInteraction();

private:
    std::unique_ptr<GizmoDrawList> draw_list;
    GizmoRenderer renderer;
    GizmoInteraction interaction;

    // GPU Color Picking 리소스
    graphics::RenderDevice* render_device = nullptr;
    SDL_GPUTransferBuffer* pick_download_buffer = nullptr; // 4바이트 DOWNLOAD용
    graphics::RID pick_texture_rid = {};                   // 1x1 R32_UINT
    EGizmoAxis hovered_axis = EGizmoAxis::None;
};
} // namespace se::editor
