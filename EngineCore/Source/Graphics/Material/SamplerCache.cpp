#include "SimpleEngine/Graphics/Material/SamplerCache.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
SamplerCache::SamplerCache(RenderDevice& in_render_device)
    : render_device(in_render_device)
{
    CreateAll();
}

SamplerCache::~SamplerCache()
{
    SDL_GPUDevice* device = render_device.GetRawDevice();
    for (SDL_GPUSampler* sampler : samplers)
    {
        if (sampler)
        {
            SDL_ReleaseGPUSampler(device, sampler);
        }
    }
}

SDL_GPUSampler* SamplerCache::Get(ESamplerType sampler_type) const
{
    const auto idx = std::to_underlying(sampler_type);
    SE_ASSERT(idx < std::to_underlying(ESamplerType::Max), "Invalid ESamplerType index.");
    return samplers[idx];
}

void SamplerCache::CreateAll()
{
    SDL_GPUDevice* device = render_device.GetRawDevice();

    // ESamplerType 순서(LinearRepeat, LinearClamp, PointRepeat, PointClamp)에 맞춰 생성
    struct SamplerDesc
    {
        SDL_GPUFilter filter;
        SDL_GPUSamplerAddressMode address_mode;
    };

    static constexpr SamplerDesc DESCS[] = {
        { .filter = SDL_GPU_FILTER_LINEAR,  .address_mode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT        }, // LinearRepeat
        { .filter = SDL_GPU_FILTER_LINEAR,  .address_mode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE }, // LinearClamp
        { .filter = SDL_GPU_FILTER_NEAREST, .address_mode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT        }, // PointRepeat
        { .filter = SDL_GPU_FILTER_NEAREST, .address_mode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE }, // PointClamp
    };
    static_assert(
        std::size(DESCS) == std::to_underlying(ESamplerType::Max),
        "DESCS entry count must match ESamplerType::Max. Add a new entry when adding a new ESamplerType."
    );

    for (usize i = 0; i < samplers.Len(); ++i)
    {
        const SDL_GPUSamplerCreateInfo info = {
            .min_filter = DESCS[i].filter,
            .mag_filter = DESCS[i].filter,
            .address_mode_u = DESCS[i].address_mode,
            .address_mode_v = DESCS[i].address_mode,
            .address_mode_w = DESCS[i].address_mode,
        };
        samplers[i] = SDL_CreateGPUSampler(device, &info);
        SE_ASSERT(samplers[i], "SDL_CreateGPUSampler failed: {}", SDL_GetError());
    }
}
} // namespace se::graphics
