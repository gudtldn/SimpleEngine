#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
{
/**
 * Entity GPU Color Picking 리소스를 소유하고 관리하는 Subsystem.
 * 1x1 R32_UINT pick 텍스처, 1x1 D24S8 depth 텍스처, Readback Transfer Buffer를 관리합니다.
 *
 * 렌더 파이프라인에서 EntityPickPass가 draw 한 뒤,
 * PerformPick()으로 GPU -> CPU readback을 수행하여 커서 아래 Entity ID를 읽습니다.
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) PickSubsystem : public SubsystemBase
{
    SE_CLASS(PickSubsystem, SubsystemBase)

public:
    PickSubsystem() = default;
    virtual ~PickSubsystem() override = default;

    PickSubsystem(const PickSubsystem&) = delete;
    PickSubsystem& operator=(const PickSubsystem&) = delete;
    PickSubsystem(PickSubsystem&&) = default;
    PickSubsystem& operator=(PickSubsystem&&) = default;

    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    /**
     * GPU Readback으로 pick 텍스처에서 encoded entity ID를 읽어 내부 상태를 갱신합니다.
     * RenderFrame() 완료 후 호출해야 합니다.
     */
    void PerformPick();

    /** 1x1 R32_UINT entity pick 텍스처 (RenderGraph ImportTexture용) */
    [[nodiscard]] SDL_GPUTexture* GetPickTexture() const;

    /** 1x1 D24_UNORM_S8_UINT depth 텍스처 (RenderGraph ImportTexture용) */
    [[nodiscard]] SDL_GPUTexture* GetPickDepthTexture() const;

    /** 마지막 PerformPick()에서 읽어낸 원본 Entity ID. Entity::Invalid이면 빈 공간. */
    [[nodiscard]] uint32 GetPickedEntityId() const { return picked_entity_id; }

    /** 커서 아래에 Entity가 있는지 여부 */
    [[nodiscard]] bool HasPickedEntity() const { return picked_entity_id != Entity::Invalid; }

private:
    graphics::RenderDevice* render_device = nullptr;

    // GPU 리소스
    SDL_GPUTransferBuffer* download_buffer = nullptr;
    graphics::RID pick_texture_rid = {}; // 1x1 R32_UINT
    graphics::RID pick_depth_rid = {};   // 1x1 D24_UNORM_S8_UINT

    // readback 결과 (디코딩된 원본 entity.id, 또는 Entity::Invalid = 미선택)
    uint32 picked_entity_id = Entity::Invalid;
};
} // namespace se::editor
