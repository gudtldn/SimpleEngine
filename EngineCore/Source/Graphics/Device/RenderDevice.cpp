#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
RenderDevice::RenderDevice(SDL_GPUDevice* raw_device)
    : raw_device(raw_device)
{
}

RenderDevice::~RenderDevice()
{
    // 지연 파괴 큐를 먼저 처리
    ProcessDeferredDestructions();

    // 남아 있는 모든 라이브 리소스를 해제
    textures.ForEach([this](RID, const TextureResource& resource)
    {
        SDL_ReleaseGPUTexture(raw_device, resource.handle);
    });
    textures.Clear();

    buffers.ForEach([this](RID, const BufferResource& resource)
    {
        SDL_ReleaseGPUBuffer(raw_device, resource.handle);
    });
    buffers.Clear();

    // SDL_GPUDevice 정리
    SDL_DestroyGPUDevice(raw_device);
}

RID RenderDevice::CreateTexture(
    const SDL_GPUTextureCreateInfo& desc,
    [[maybe_unused]] const char* debug_name
)
{
    SDL_GPUTexture* raw = SDL_CreateGPUTexture(raw_device, &desc);
    if (!raw)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUTexture failed: {}", SDL_GetError());
        return {};
    }

#if SE_ENABLE_DEBUG_TOOLS
    // props에 이름이 설정되지 않은 경우에만 debug_name을 사용
    if (debug_name && !(desc.props && SDL_GetStringProperty(desc.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, nullptr)))
    {
        SDL_SetGPUTextureName(raw_device, raw, debug_name);
    }
#endif

    return textures.Insert({
        .handle = raw,
        .width = desc.width,
        .height = desc.height,
        .format = desc.format,
    });
}

RID RenderDevice::CreateBuffer(
    const SDL_GPUBufferCreateInfo& desc,
    [[maybe_unused]] const char* debug_name
)
{
    SDL_GPUBuffer* raw = SDL_CreateGPUBuffer(raw_device, &desc);
    if (!raw)
    {
        ConsoleLog(ELogLevel::Error, "SDL_CreateGPUBuffer failed: {}", SDL_GetError());
        return {};
    }

#if SE_ENABLE_DEBUG_TOOLS
    // props에 이름이 설정되지 않은 경우에만 debug_name을 사용
    if (debug_name && !(desc.props && SDL_GetStringProperty(desc.props, SDL_PROP_GPU_BUFFER_CREATE_NAME_STRING, nullptr)))
    {
        SDL_SetGPUBufferName(raw_device, raw, debug_name);
    }
#endif

    return buffers.Insert({
        .handle = raw,
        .size = desc.size,
        .usage = desc.usage,
    });
}

Optional<TextureResource> RenderDevice::GetTexture(RID rid) const
{
    return textures.Get(rid).Copy();
}

Optional<BufferResource> RenderDevice::GetBuffer(RID rid) const
{
    return buffers.Get(rid).Copy();
}

bool RenderDevice::IsValidTexture(RID rid) const
{
    return textures.IsValidRID(rid);
}

bool RenderDevice::IsValidBuffer(RID rid) const
{
    return buffers.IsValidRID(rid);
}

void RenderDevice::DestroyTexture(RID rid)
{
    if (auto resource = textures.Get(rid))
    {
        deferred_texture_destroys.Push(resource->handle);
        textures.Remove(rid);
    }
}

void RenderDevice::DestroyBuffer(RID rid)
{
    if (auto resource = buffers.Get(rid))
    {
        deferred_buffer_destroys.Push(resource->handle);
        buffers.Remove(rid);
    }
}

void RenderDevice::ProcessDeferredDestructions()
{
    for (SDL_GPUTexture* texture : deferred_texture_destroys)
    {
        SDL_ReleaseGPUTexture(raw_device, texture);
    }
    deferred_texture_destroys.Clear();

    for (SDL_GPUBuffer* buffer : deferred_buffer_destroys)
    {
        SDL_ReleaseGPUBuffer(raw_device, buffer);
    }
    deferred_buffer_destroys.Clear();
}
} // namespace se
