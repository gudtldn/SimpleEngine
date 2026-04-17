#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Graphics/Device/RenderDevice.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
{
/**
 * 1x1 R32_UINT 텍스처 + GPU->CPU Readback 버퍼를 캡슐화한 유틸리티.
 * Entity Picking과 Gizmo Picking 양쪽에서 동일한 패턴을 재사용합니다.
 */
class SE_EDITOR_API GpuPickBuffer
{
public:
    GpuPickBuffer() = default;
    ~GpuPickBuffer() = default;

    GpuPickBuffer(const GpuPickBuffer&) = delete;
    GpuPickBuffer& operator=(const GpuPickBuffer&) = delete;
    GpuPickBuffer(GpuPickBuffer&&) = default;
    GpuPickBuffer& operator=(GpuPickBuffer&&) = default;

    /** 1x1 R32_UINT 텍스처와 4바이트 download buffer를 생성합니다. */
    bool Create(graphics::RenderDevice& device);

    /** GPU 리소스를 해제합니다. */
    void Destroy();

    /**
     * 텍스처에서 1 픽셀을 CPU로 readback합니다.
     * fence 대기를 포함하는 동기 호출입니다.
     * @return 읽은 uint32 값. 실패 시 0.
     */
    uint32 PerformReadback();

    /** RenderGraph ImportTexture용 텍스처 핸들 */
    [[nodiscard]] SDL_GPUTexture* GetTexture() const;

    [[nodiscard]] bool IsValid() const { return texture_rid && download_buffer; }

    [[nodiscard]] explicit operator bool() const { return IsValid(); }

private:
    graphics::RenderDevice* render_device = nullptr;
    graphics::RID texture_rid = {};
    SDL_GPUTransferBuffer* download_buffer = nullptr;
};
} // namespace se::editor
