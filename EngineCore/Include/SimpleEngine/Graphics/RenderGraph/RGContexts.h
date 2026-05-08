#pragma once

#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
// forward declarations
struct ComputePipelineCreateInfo;
struct GraphicsPipelineCreateInfo;
struct RGPassNode;
class PSOManager;
class RenderGraphBuilder;

/**
 * Setup()에 전달되는 Context
 * Setup에서의 리소스 생성은 불가능 하며, Read/Write 선언만 허용합니다.
 */
class SE_CORE_API RGSetupContext
{
public:
    explicit RGSetupContext(RGPassNode& in_pass_node);

    /** 핸들이 가리키는 택스처를 이 패스가 읽도록 선언합니다. */
    void Read(RGTextureHandle handle);

    /** 핸들이 가리키는 버퍼를 이 패스가 읽도록 선언합니다. */
    void Read(RGBufferHandle handle);

    /** 핸들이 가리키는 택스처를 이 패스가 쓰도록 선언합니다. */
    void Write(RGTextureHandle handle);

    /** 핸들이 가리키는 버퍼를 이 패스가 쓰도록 선언합니다. */
    void Write(RGBufferHandle handle);

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

    /** 커맨드 버퍼에 디버그 서브그룹을 시작합니다. Debug/Development 빌드에서만 동작합니다. */
    void PushDebugGroup(const char* name);

    /** 가장 최근 Push한 디버그 서브그룹을 종료합니다. */
    void PopDebugGroup();

    /** 커맨드 버퍼에 단발성 디버그 라벨을 삽입합니다. */
    void InsertDebugLabel(const char* text);

private:
    SDL_GPUCommandBuffer* command_buffer;
    PSOManager& pso_manager;
    const RenderGraphBuilder& builder_ref;
};
} // namespace se
