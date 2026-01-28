// NOLINTBEGIN(*-reserved-identifier)
#include "Asset/Pipeline/AssetImporter.h"
#include "Core/Container/Queue.h"
#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
Array<std::shared_ptr<IAsset>> AssetImporter::Import(
    const std::filesystem::path& file_path,
    const ImportConfig& import_config,
    Optional<const PipelineProcessorStack&> processor_stack
)
{
    ZoneScopedN("AssetImporter::Import");
#if TRACY_ENABLE
        const std::string filename = file_path.filename().string();
        ZoneText(filename.c_str(), filename.size());
#endif

    // ---------------------------------------------------------
    // 1단계: Translator 실행 (File -> Raw Nodes)
    // ---------------------------------------------------------
    const Optional translator_opt = FindTranslator(file_path);
    if (!translator_opt)
    {
        ConsoleLog(ELogLevel::Error, "No suitable translator found for file: {}", file_path.generic_string());
        return {};
    }

    PipelineNodeContainer container;
    {
        ZoneScopedN("Translator::Translate");
        translator_opt->Translate(file_path, import_config, container);
    }

    // ---------------------------------------------------------
    // 2단계: Pipeline Stack 실행 (Node Modification)
    // ---------------------------------------------------------
    if (processor_stack)
    {
        ZoneScopedN("ProcessorStack::ExecuteStack");
        processor_stack->ExecuteStack(container);
    }

    // ---------------------------------------------------------
    // 3단계: 의존성 정렬 (Dependency Sorting)
    // ---------------------------------------------------------
    const Array<PipelineBaseNode*> sorted_nodes = [&container]
    {
        ZoneScopedN("SortNodesByDependency"); // NOLINT(*-lambda-function-name)
        return SortNodesByDependency(container);
    }();

    // ---------------------------------------------------------
    // 4단계: Factory 실행 (Nodes -> Assets)
    // ---------------------------------------------------------
    HashMap<Guid, std::shared_ptr<IAsset>> created_assets_map;
    Array<std::shared_ptr<IAsset>> created_assets;

    // Factory가 참조할 Context 생성
    const PipelineImportContext context{
        .container = container,
        .created_assets = created_assets_map
    };

    {
        ZoneScopedN("Factory::CreateAssets");
        for (PipelineBaseNode* node : sorted_nodes)
        {
            ZoneScopedN("Process Node");
#if TRACY_ENABLE
            const std::string_view node_name = node->GetTypeId().GetName();
            ZoneText(node_name.data(), node_name.size());
#endif

            [&]
            {
                // 해당 노드를 처리할 수 있는 팩토리를 검색
                // TODO: 반복문이 좀 비효율적인데, 추후 개선
                for (const auto& factory : factories)
                {
                    if (factory->CanCreateAsset(node))
                    {
                        ZoneScopedN("Factory::CreateAsset"); // NOLINT(*-lambda-function-name)
                        if (std::shared_ptr<IAsset> new_asset = factory->CreateAsset(node, context))
                        {
                            created_assets_map.Insert(node->GetUid(), new_asset);
                            created_assets.Push(new_asset);
                        }
                        return;
                    }
                }
                ConsoleLog(ELogLevel::Warning, "No factory found for node type: {}", node->GetTypeId().GetName());
            }();
        }
    }

    return created_assets;
}

Optional<IPipelineTranslator&> AssetImporter::FindTranslator(const std::filesystem::path& file_path) const
{
    const String ext = utility::ToString(file_path.extension().u8string()).ToLower();
    for (const auto& translator : translators)
    {
        if (translator->CanTranslate(ext))
        {
            return *translator;
        }
    }
    return std::nullopt;
}

Array<PipelineBaseNode*> AssetImporter::SortNodesByDependency(const PipelineNodeContainer& container)
{
    const auto& all_nodes = container.GetAllNodes();

    Array<PipelineBaseNode*> result;
    result.Reserve(all_nodes.Len());

    HashMap<Guid, Array<Guid>> dependents_map; // 각 노드의 의존성 목록
    HashMap<Guid, int32> in_degrees;           // 남은 의존성 개수

    // 그래프 구축 및 진입 차수(In-Degree) 계산
    Queue<PipelineBaseNode*> ready_queue;
    for (const auto& [node_id, node] : all_nodes)
    {
        // 현재 노드가 필요로 하는 의존성을 가져옴
        Array<Guid> dependencies;
        node->GetFactoryDependencies(dependencies);

        int32 current_in_degree = 0;
        for (const Guid& dependency_id : dependencies)
        {
            // 유효한 의존성인지 확인
            if (container.Contains(dependency_id))
            {
                // 역방향 그래프 구축: 의존성(부모) -> 의존자(자식) 관계 저장
                dependents_map[dependency_id].Push(node_id);
                ++current_in_degree;
            }
        }

        in_degrees.Insert(node_id, current_in_degree);

        // 진입 차수가 0인 노드는 queue에 추가
        if (current_in_degree == 0)
        {
            ready_queue.Push(node.get());
        }
    }

    // 위상 정렬 수행
    while (Optional node_opt = ready_queue.Pop())
    {
        PipelineBaseNode* current_node = *node_opt;
        result.Push(current_node);

        // current_node를 기다리던 다른 노드들을 확인
        if (Optional dependents_opt = dependents_map.Find(current_node->GetUid()))
        {
            for (const Guid& dependent_id : *dependents_opt)
            {
                // 해당 노드의 남은 의존성 개수 감소
                int32& degree = in_degrees[dependent_id];
                degree -= 1;

                // 모든 의존성이 해결되었다면 queue에 추가
                if (degree == 0)
                {
                    if (Optional next_node_opt = container.GetNode(dependent_id))
                    {
                        ready_queue.Push(&next_node_opt.Value());
                    }
                }
            }
        }
    }

    // 순환 의존성(Cycle) 감지
    if (result.Len() != all_nodes.Len())
    {
        ConsoleLog(ELogLevel::Error, "Cyclic dependency detected! Sorted {} nodes out of {}.", result.Len(), all_nodes.Len());
        ConsoleLog(ELogLevel::Error, "The following nodes represent the cycle or are stuck:");

        for (const auto& [node_id, degree] : in_degrees)
        {
            if (degree > 0)
            {
                if (const Optional node_opt = container.GetNode(node_id))
                {
                    ConsoleLog(ELogLevel::Error, "- {} (Type: {}, Remaining Deps: {})",
                        node_opt->GetDisplayName(),
                        node_opt->GetTypeId().GetName(),
                        degree
                    );
                }
            }
        }

        SE_BREAKPOINT();
        return {};
    }

    return result;
}
}  // namespace se::asset
// NOLINTEND(*-reserved-identifier)
