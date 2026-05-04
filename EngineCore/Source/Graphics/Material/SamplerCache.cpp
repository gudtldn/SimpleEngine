#include "SimpleEngine/Graphics/Material/SamplerCache.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
SamplerCache::SamplerCache(RenderDevice& in_render_device)
    : render_device(in_render_device)
{
    CreateAll();
}

SamplerCache::~SamplerCache()
{
    for (SDL_GPUSampler* sampler : samplers)
    {
        if (sampler)
        {
            SDL_ReleaseGPUSampler(render_device.GetRawDevice(), sampler);
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
    // ESamplerType 순서(LinearRepeat, LinearClamp, PointRepeat, PointClamp)에 맞춰 생성
    struct SamplerDesc
    {
        SDL_GPUFilter filter;
        SDL_GPUSamplerAddressMode address_mode;
    };

    static constexpr SamplerDesc DESCS[] = {
        { .filter = SDL_GPU_FILTER_LINEAR,  .address_mode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT        }, // 0: LinearRepeat (일반 3D)
        { .filter = SDL_GPU_FILTER_LINEAR,  .address_mode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE }, // 1: LinearClamp  (UI/마스크)
        { .filter = SDL_GPU_FILTER_NEAREST, .address_mode = SDL_GPU_SAMPLERADDRESSMODE_REPEAT        }, // 2: PointRepeat  (도트 아트)
        { .filter = SDL_GPU_FILTER_NEAREST, .address_mode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE }, // 3: PointClamp   (도트 UI)
    };
    static_assert(
        std::size(DESCS) == std::to_underlying(ESamplerType::Max),
        "DESCS entry count must match ESamplerType::Max. Add a new entry when adding a new ESamplerType."
    );

    for (usize i = 0; i < samplers.Len(); ++i)
    {
        const SDL_GPUSamplerCreateInfo info = {
            // 1. 기본 필터링 (확대/축소 시 픽셀 혼합 방식)
            .min_filter = DESCS[i].filter,
            .mag_filter = DESCS[i].filter,

            // 2. 밉맵(Mipmap) 필터링 모드
            // LINEAR로 설정해야 거리에 따라 LOD가 바뀔 때 층과 층 사이를 부드럽게 섞음 (Trilinear Filtering)
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,

            // 3. 어드레싱 모드 (UV 좌표가 0.0 ~ 1.0 범위를 벗어났을 때의 처리)
            .address_mode_u = DESCS[i].address_mode,
            .address_mode_v = DESCS[i].address_mode,
            .address_mode_w = DESCS[i].address_mode,

            // 4. LOD (Level of Detail) 접근 제한
            .mip_lod_bias = 0.0f,                  // 계산된 LOD 값 오프셋 (기본 0, Metal에서는 무시됨)
            .max_anisotropy = 16.0f,               // 비등방성 필터링 허용 최대치 (최신 GPU 표준 16x)
            .compare_op = SDL_GPU_COMPAREOP_NEVER, // 섀도우 맵핑(PCF) 전용 비교 연산자 (일반 텍스처는 NEVER)
            .min_lod = 0.0f,                       // 가장 선명한 텍스처(Mip 0)부터 사용 허용
            .max_lod = 1000.0f,                    // 가장 흐릿한 끝층까지 제한 없이 허용 (텍스처 자체의 num_levels에 자동 맞춰짐)

            // 5. 기능 활성화 스위치 (Boolean)
            // 비등방성(Anisotropy): 카메라가 바닥/벽을 비스듬히 볼 때 텍스처가 뭉개지는 현상 방지.
            // 단, 도트(Nearest) 픽셀 아트에서는 그래픽이 깨지므로 Linear 필터일 때만 활성화.
            .enable_anisotropy = (DESCS[i].filter == SDL_GPU_FILTER_LINEAR),

            // 비교(Compare) 샘플링: 하드웨어 가속 부드러운 그림자(PCF) 등을 위해 사용. 일반 PBR 렌더링에선 false
            .enable_compare = false, // 깊이 섀도우 맵핑 등에 사용되는 옵션 (현재 미사용)
        };

        samplers[i] = SDL_CreateGPUSampler(render_device.GetRawDevice(), &info);
        SE_ASSERT(samplers[i], "SDL_CreateGPUSampler failed: {}", SDL_GetError());
    }
}
} // namespace se
