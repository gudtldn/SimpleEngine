#pragma once
#include <memory>

#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Reflection/TypeSignature.h"
#include "SimpleEngine/Rendering/RenderGraph/RGResourceHandle.h"
#include "SimpleEngine/Rendering/RenderGraph/RGResources.h"
#include "SimpleEngine/Rendering/RenderPass/IRenderPass.h"


namespace se::graphics
{
// forward declaration
struct GraphicsPipelineCreateInfo;
struct ComputePipelineCreateInfo;
class PSOManager;
class RenderGraphBuilder;
class RGExecutionContext;


/** 그래프 내의 렌더 패스를 표현하는 내부 구조체 */
struct RGPassNode
{
    StringName name;
    std::unique_ptr<IRenderPass> pass_object;
    HashSet<RGResourceHandle> reads;
    HashSet<RGResourceHandle> writes;

    // RenderGraph::Compile() 단계에서 채워질 정보들
    bool culled = true; // 이번 프레임에서 사용 안하는지 여부 (Compile때 false로 변경)
};

/** 그래프 내의 텍스처 리소스를 표현하는 내부 구조체 */
struct RGResourceNode
{
    StringName name;
    std::unique_ptr<IRGResource> resource;

    // RenderGraph::Compile() 단계에서 채워질 정보들
    const RGPassNode* writer = nullptr; // 이 리소스를 쓰는 패스 (프레임당 하나여야 함)

    // lifetime 정보 (위상 정렬 후 계산됨)
    uint32 first_user_pass_index = std::numeric_limits<uint32>::max();
    uint32 last_user_pass_index = 0;
};


/**
 * 렌더링 파이프라인의 구성, 최적화, 실행을 관리하는 핵심 클래스
 */
class SE_CORE_API RenderGraph
{
    friend class RenderGraphBuilder;
    friend class RGExecutionContext;

public:
    explicit RenderGraph(SDL_GPUDevice* in_device);
    ~RenderGraph();

    // 복사 생성자는 제거
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) noexcept = default;
    RenderGraph& operator=(RenderGraph&&) noexcept = default;

public:
    /**
     * 새로운 RenderPass를 그래프에 추가합니다.
     * @tparam PassType IRenderPass를 상속받아 구현된 패스 객체의 unique_ptr
     * @return 추가된 패스에 대한 참조
     */
    template <typename PassType, typename... Args>
        requires std::derived_from<PassType, IRenderPass>
    PassType& AddPass(Args&&... args);

    void Compile();
    void Execute(SDL_GPUCommandBuffer* cmd, PSOManager& pso_manager);
    void Clear();

public:
    RGResourceHandle ImportTexture(const StringName& name, SDL_GPUTexture* texture);
    RGResourceHandle ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer);

private:
    [[nodiscard]] RGResourceHandle GetResourceHandleByName(const StringName& name);
    RGResourceHandle RegisterResource(RGResourceNode&& node);

private:
    SDL_GPUDevice* device;
    FrameResourcePool resource_pool;

    // StringName으로 리소스 핸들을 찾기 위한 Map
    HashMap<StringName, RGResourceHandle> resource_name_map;

    Array<RGPassNode> pass_nodes;
    Array<RGResourceNode> resource_nodes;

    // 컴파일 후 정렬된 Pass의 순서
    Array<const RGPassNode*> compiled_passes;
};

/**
 * IRenderPass::Setup() 내에서 RenderGraph의 상태를 안전하게 조작하기 위한 빌더 클래스
 */
class SE_CORE_API RenderGraphBuilder
{
public:
    RenderGraphBuilder(RenderGraph& in_graph, RGPassNode& in_pass)
        : graph_ref(in_graph)
        , pass_node_ref(in_pass)
    {
    }

public:
    [[nodiscard]] RGResourceHandle GetResourceHandleByName(const StringName& name) const;

    RGResourceHandle CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& description);
    RGResourceHandle CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& description);

    void Read(RGResourceHandle handle);
    void Write(RGResourceHandle handle);

private:
    RenderGraph& graph_ref;
    RGPassNode& pass_node_ref;
};

/**
 * IRenderPass::Execute()에 전달되어 사용되는 컨텍스트 클래스
 */
class SE_CORE_API RGExecutionContext
{
public:
    RGExecutionContext(SDL_GPUCommandBuffer* in_cmd, PSOManager& in_pso_manager, const RenderGraph& in_graph)
        : command_buffer(in_cmd)
        , pso_manager(in_pso_manager)
        , graph_ref(in_graph)
    {
    }

public:
    [[nodiscard]] SDL_GPUCommandBuffer* GetCommandBuffer() const { return command_buffer; }

    [[nodiscard]] SDL_GPUTexture* GetActualTexture(RGResourceHandle handle) const;
    [[nodiscard]] SDL_GPUBuffer* GetActualBuffer(RGResourceHandle handle) const;

    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info);

private:
    SDL_GPUCommandBuffer* command_buffer;
    PSOManager& pso_manager;
    const RenderGraph& graph_ref;
};


template <typename PassType, typename... Args>
    requires std::derived_from<PassType, IRenderPass>
PassType& RenderGraph::AddPass(Args&&... args)
{
    using namespace se;

    auto pass_ptr = std::make_unique<PassType>(std::forward<Args>(args)...);
    PassType* raw_ptr = pass_ptr.get();

    RGPassNode& node = pass_nodes.Emplace();
    node.name = StringName{ GetFullTypeName<PassType>() };
    node.pass_object = std::move(pass_ptr);

    return *raw_ptr;
}
}  // namespace se::graphics
