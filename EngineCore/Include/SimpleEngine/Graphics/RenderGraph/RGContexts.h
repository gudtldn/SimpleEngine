#pragma once

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"

#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
// forward declarations
struct ComputePipelineCreateInfo;
struct GraphicsPipelineCreateInfo;
class PSOManager;
struct RGPassNode;
class RenderGraphBuilder;

/**
 * Setup()에 전달되는 Context
 * Setup에서의 리소스 생성은 불가능 하며, Read/Write 선언만 허용합니다.
 */
class SE_CORE_API RGSetupContext
{
public:
    explicit RGSetupContext(RGPassNode& in_pass_node);

    void Read(RGTextureHandle handle);
    void Read(RGBufferHandle handle);

    /**
     * 핸들이 가리키는 리소스를 이 패스가 쓰도록 선언합니다.
     * @return 버전이 증가된 새 핸들. Execute()에서 이 핸들로 실제 리소스를 조회해야 합니다.
     */
    [[nodiscard]] RGTextureHandle Write(RGTextureHandle handle);
    [[nodiscard]] RGBufferHandle Write(RGBufferHandle handle);

private:
    RGPassNode& pass_node_ref;
};

/**
 * Execute()에 전달되는 Context
 */
class SE_CORE_API RGExecutionContext
{
public:
    RGExecutionContext(
        SDL_GPUCommandBuffer* in_cmd,
        PSOManager& in_pso_manager,
        const RenderGraphBuilder& in_builder
    );

    [[nodiscard]] SDL_GPUTexture* GetActualTexture(RGTextureHandle handle) const;
    [[nodiscard]] SDL_GPUBuffer* GetActualBuffer(RGBufferHandle handle) const;

    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info);

    [[nodiscard]] SDL_GPUCommandBuffer* GetCommandBuffer() const { return command_buffer; }

private:
    SDL_GPUCommandBuffer* command_buffer;
    PSOManager& pso_manager;
    const RenderGraphBuilder& builder_ref;
};
} // namespace se::graphics