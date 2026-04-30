#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Graphics/RenderGraph/FrameResourcePool.h"
#include "SimpleEngine/Graphics/RenderGraph/RGNodeTypes.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
class PSOManager;
class RenderDevice;
class RenderGraphBuilder;

/**
 * RenderGraphBuilder를 컴파일하고 실행하는 클래스
 */
class SE_CORE_API RenderGraphExecutor
{
public:
    explicit RenderGraphExecutor(RenderDevice& in_render_device);
    ~RenderGraphExecutor();

    RenderGraphExecutor(const RenderGraphExecutor&) = delete;
    RenderGraphExecutor& operator=(const RenderGraphExecutor&) = delete;

public:
    /**
     * builder를 컴파일하고 실행합니다.
     * 완료 후 builder.Clear()를 호출합니다.
     */
    void Execute(RenderGraphBuilder& builder, SDL_GPUCommandBuffer* cmd, PSOManager& pso_manager);

    /** 임시 리소스 풀의 idle 카운터 갱신 및 미사용 리소스를 정리합니다. */
    void UpdateResourcePool();

private:
    void Compile(RenderGraphBuilder& builder);

    RenderDevice* render_device;
    FrameResourcePool resource_pool;

    // Compile() 결과 (매 프레임 재구성)
    Array<const RGPassNode*> compiled_passes;
    Array<Array<usize>> resources_to_realize;
    Array<Array<usize>> resources_to_unrealize;
};
} // namespace se
