#pragma once

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"

#include "SDL3/SDL_gpu.h"

#include <utility>


namespace se
{
// forward declaration
class RenderDevice;

/**
 * ESamplerType별 GPU Sampler를 미리 생성하고 캐싱하는 클래스
 */
class SE_CORE_API SamplerCache
{
public:
    explicit SamplerCache(RenderDevice& in_render_device);
    ~SamplerCache();

    SamplerCache(const SamplerCache&) = delete;
    SamplerCache& operator=(const SamplerCache&) = delete;
    SamplerCache(SamplerCache&&) = delete;
    SamplerCache& operator=(SamplerCache&&) = delete;

public:
    /** ESamplerType에 해당하는 GPU Sampler를 반환합니다. (항상 유효한 포인터) */
    [[nodiscard]] SDL_GPUSampler* Get(ESamplerType sampler_type) const;

private:
    /** 모든 샘플러를 미리 생성합니다. */
    void CreateAll();

private:
    RenderDevice& render_device;
    FixedArray<SDL_GPUSampler*, std::to_underlying(ESamplerType::Max)> samplers = {};
};
} // namespace se
