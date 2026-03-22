#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeSignature.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/RenderGraph/RGNodeTypes.h"
#include "SimpleEngine/Graphics/RenderGraph/RGResourceHandle.h"

#include <memory>


namespace se::graphics
{
class RGSetupContext;
class RGExecutionContext;
class RenderGraphExecutor;

/**
 * 프레임 단위의 RenderGraph 구성을 담당하는 클래스
 * Import/Create/AddPass API를 통해 패스와 리소스를 등록합니다.
 */
class SE_CORE_API RenderGraphBuilder
{
    friend class RGSetupContext;
    friend class RGExecutionContext;
    friend class RenderGraphExecutor;

public:
    RenderGraphBuilder() = default;
    ~RenderGraphBuilder() = default;

    // 이동만 가능
    RenderGraphBuilder(const RenderGraphBuilder&) = delete;
    RenderGraphBuilder& operator=(const RenderGraphBuilder&) = delete;
    RenderGraphBuilder(RenderGraphBuilder&&) = default;
    RenderGraphBuilder& operator=(RenderGraphBuilder&&) = default;

public:
    /** 외부 텍스처(예: 스왑체인)를 그래프에 등록합니다. */
    RGTextureHandle ImportTexture(const StringName& name, SDL_GPUTexture* texture);
    RGBufferHandle ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer);

    /**
     * 프레임 수명의 임시 텍스처/버퍼를 그래프에 등록합니다.
     * @note 실제 GPU 리소스 할당은 Executor가 실행 시점에 담당합니다.
     */
    RGTextureHandle CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& desc);
    RGBufferHandle CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& desc);

    /** 렌더 패스를 그래프에 추가합니다. */
    template <typename PassType, typename... Args>
        requires std::derived_from<PassType, RenderPassBase>
    PassType& AddPass(Args&&... args);

    /**
     * 다음 프레임을 위해 그래프 상태를 초기화합니다.
     * Executor가 Execute()를 호출후 실행합니다.
     */
    void Clear();

private:
    void AddPassInternal(StringName name, std::unique_ptr<RenderPassBase> pass);

    [[nodiscard]] RGTextureHandle RegisterTextureSlot(const StringName& name);
    [[nodiscard]] RGBufferHandle RegisterBufferSlot(const StringName& name);

    HashMap<StringName, uint32> resource_name_map;
    Array<RGResourceNode> resource_nodes;
    Array<RGPassNode> pass_nodes;
};

template <typename PassType, typename... Args>
    requires std::derived_from<PassType, RenderPassBase>
PassType& RenderGraphBuilder::AddPass(Args&&... args)
{
    auto pass_ptr = std::make_unique<PassType>(std::forward<Args>(args)...);
    PassType* raw_ptr = pass_ptr.get();
    AddPassInternal(StringName{ GetFullTypeName<PassType>() }, std::move(pass_ptr));
    return *raw_ptr;
}
} // namespace se::graphics