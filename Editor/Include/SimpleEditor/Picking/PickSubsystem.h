#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/ECS/EntityPickId.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
{
/**
 * Entity GPU Color Picking 리소스를 소유하고 관리하는 Subsystem
 * ForwardScenePass의 MRT entity_id 텍스처(viewport 해상도)를 소유하며,
 * PerformPick()으로 커서 위치 1픽셀을 GPU -> CPU readback하여 Entity ID를 읽습니다.
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) PickSubsystem : public SubsystemBase
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
     * 뷰포트 크기에 맞는 entity_id 텍스처를 반환합니다.
     * 필요 시 텍스처를 생성/재생성합니다. (RenderGraph ImportTexture용)
     */
    [[nodiscard]] SDL_GPUTexture* GetOrCreateEntityIdTexture(u32 width, u32 height);

    /**
     * entity_id 텍스처에서 커서 위치의 1픽셀을 GPU readback으로 읽어 내부 상태를 갱신합니다.
     * RenderFrame() 완료 후 호출해야 합니다.
     */
    void PerformPick(const Vector2f& cursor_pos);

    /** 마지막 PerformPick() 결과 */
    [[nodiscard]] EntityPickId GetPickId() const { return pick_id; }

private:
    /** 뷰포트 리사이즈 시 entity_id 텍스처를 재생성합니다. */
    void EnsureSize(u32 width, u32 height);

    /** entity_id 텍스처 raw 포인터 */
    [[nodiscard]] SDL_GPUTexture* GetEntityIdTexture() const;

private:
    RenderDevice* render_device = nullptr;

    // GPU 리소스
    RID entity_id_texture_rid = {};         // viewport 해상도 R32_UINT
    SDL_GPUTransferBuffer* download_buffer = nullptr; // 4바이트 readback

    // 텍스처 크기 캐시
    u32 texture_width = 0;
    u32 texture_height = 0;

    // readback 결과
    EntityPickId pick_id;
};
} // namespace se::editor
