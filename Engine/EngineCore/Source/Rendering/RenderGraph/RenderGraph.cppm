export module SE.Rendering:RenderGraph;
export import :RenderGraph.RGResoueceHandle;
export import :RenderGraph.RGResources;
import :Manager.PSOManager;

import SE.Core;
import SE.Types;
import SE.Utility;
import SE.Interface.IRenderPass;
import std;

import <SDL3/SDL_gpu.h>;


namespace se::rendering::render_graph
{
// forward declaration
export class RenderGraphBuilder;


/** 그래프 내의 텍스처 리소스를 표현하는 내부 구조체 */
struct RGResourceNode
{
    StringName name;
    std::unique_ptr<IRGResource> resource;
    // TODO: 생명주기 관리를 위한 추가 정보 (first_user, last_user 등)
};

/** 그래프 내의 렌더 패스를 표현하는 내부 구조체 */
struct RGPassNode
{
    StringName name;
    std::unique_ptr<IRenderPass> pass_object;
    std::vector<RGResourceHandle> reads;
    std::vector<RGResourceHandle> writes;
};


/**
 * 렌더링 파이프라인의 구성, 최적화, 실행을 관리하는 핵심 클래스
 */
export class RenderGraph
{
    friend class RenderGraphBuilder;

public:
    RenderGraph(SDL_GPUDevice* in_device)
        : device(in_device)
    {
    }

    ~RenderGraph()
    {
        Clear();
    }

    // 복사 생성자는 제거
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = default;
    RenderGraph& operator=(RenderGraph&&) = default;

    /**
     * 새로운 RenderPass를 그래프에 추가합니다.
     * @tparam PassType IRenderPass를 상속받아 구현된 패스 객체의 unique_ptr
     * @return 추가된 패스에 대한 참조
     */
    template <typename PassType, typename... Args>
        requires std::derived_from<PassType, IRenderPass>
    PassType& AddPass(Args&&... args);

    void Compile();
    void Execute(SDL_GPUCommandBuffer* cmd, manager::PSOManager& pso_manager);
    void Clear();

    SDL_GPUTexture* GetActualTexture(RGResourceHandle handle) const;
    SDL_GPUBuffer* GetActualBuffer(RGResourceHandle handle) const;

    RGResourceHandle ImportTexture(const StringName& name, SDL_GPUTexture* texture);
    RGResourceHandle ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer);

    Optional<RGResourceHandle> FindResource(const StringName& name) const;

private:
    RGResourceHandle RegisterResource(RGResourceNode&& node);

private:
    SDL_GPUDevice* device;
    std::vector<RGPassNode> pass_nodes;
    std::vector<RGResourceNode> resource_nodes;

    // 컴파일 후 정렬된 Pass의 순서
    std::vector<const RGPassNode*> compiled_passes;
};

/**
 * IRenderPass::Setup() 내에서 RenderGraph의 상태를 안전하게 조작하기 위한 빌더 클래스
 */
class RenderGraphBuilder
{
public:
    RenderGraphBuilder(RenderGraph& in_graph, RGPassNode& in_pass)
        : graph_ref(in_graph), pass_node_ref(in_pass)
    {
    }

    RGResourceHandle CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& description);
    RGResourceHandle CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& description);

    RGResourceHandle ImportTexture(const StringName& name, SDL_GPUTexture* texture);
    RGResourceHandle ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer);

    Optional<RGResourceHandle> FindResource(const StringName& name) const;

    void Read(RGResourceHandle handle);
    void Write(RGResourceHandle handle);

private:
    RenderGraph& graph_ref;
    RGPassNode& pass_node_ref;
};

/**
 * IRenderPass::Execute()에 전달되어 사용되는 컨텍스트 클래스
 */
export class RGExecutionContext
{
public:
    RGExecutionContext(SDL_GPUCommandBuffer* in_cmd, manager::PSOManager& in_pso_manager, const RenderGraph& in_graph)
        : command_buffer(in_cmd)
        , pso_manager(in_pso_manager)
        , graph_ref(in_graph)
    {
    }

    [[nodiscard]] SDL_GPUCommandBuffer* GetCommandBuffer() const { return command_buffer; }

    [[nodiscard]] SDL_GPUTexture* GetActualTexture(RGResourceHandle handle) const;
    [[nodiscard]] SDL_GPUBuffer* GetActualBuffer(RGResourceHandle handle) const;

    [[nodiscard]] SDL_GPUGraphicsPipeline* GetOrCreateGraphicsPipeline(const GraphicsPipelineCreateInfo& create_info);
    [[nodiscard]] SDL_GPUComputePipeline* GetOrCreateComputePipeline(const ComputePipelineCreateInfo& create_info);

private:
    SDL_GPUCommandBuffer* command_buffer;
    manager::PSOManager& pso_manager;
    const RenderGraph& graph_ref;
};


template <typename PassType, typename... Args>
    requires std::derived_from<PassType, IRenderPass>
PassType& RenderGraph::AddPass(Args&&... args)
{
    auto pass_ptr = std::make_unique<PassType>(std::forward<Args>(args)...);
    PassType* raw_ptr = pass_ptr.get();

    RGPassNode& node = pass_nodes.emplace_back();
    node.name = StringName(utility::string_utils::ToU8String(core::reflection::GetTypeSignature<PassType>()));
    node.pass_object = std::move(pass_ptr);

    return *raw_ptr;
}
}
