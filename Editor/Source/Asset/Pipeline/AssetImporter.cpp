// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"

#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
bool AssetImporter::CanImport(const Path& file_path) const
{
    return FindTranslator(file_path).HasValue();
}

HashSet<StringView> AssetImporter::GetAllSupportedExtensions() const
{
    HashSet<StringView> result;
    for (const auto& translator : translators)
    {
        for (const StringView ext : translator->GetSupportedExtensions())
        {
            result.Insert(ext);
        }
    }
    return result;
}

Expected<ImportResult, ImportError> AssetImporter::Import(
    const Path& file_path,
    const ImportProfile& import_profile,
    Optional<const PipelineProcessorStack&> processor_stack
)
{
    ZoneScopedN("AssetImporter::Import");
#if TRACY_ENABLE
    const String filename = file_path.FileName().ValueOr("Unknown");
    ZoneText(filename.CStr(), filename.ByteLen());
#endif

    // ---------------------------------------------------------
    // 1단계: Translator 실행 (File -> Raw Nodes)
    // ---------------------------------------------------------
    const Optional translator_opt = FindTranslator(file_path);
    if (!translator_opt)
    {
        ConsoleLog(ELogLevel::Error, "No suitable translator found for file: {}", file_path);
        return Unexpected(ImportError{
            ImportError::ECode::NoTranslator,
            "No suitable translator found",
            file_path
        });
    }

    PipelineNodeContainer container;
    {
        ZoneScopedN("Translator::Translate");
        translator_opt->Translate(file_path, import_profile, container);
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

    // 순환 의존성 감지 시 실패 반환
    if (sorted_nodes.IsEmpty() && !container.GetAllNodes().IsEmpty())
    {
        return Unexpected(ImportError{
            ImportError::ECode::CyclicDependency,
            "Cyclic dependency detected in pipeline nodes",
            file_path
        });
    }

    // ---------------------------------------------------------
    // 4단계: Factory 실행 (Nodes -> Assets)
    // ---------------------------------------------------------
    ImportResult::Builder result_builder;
    HashMap<Guid, std::shared_ptr<asset::AssetBase>> created_assets_map;

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
            const StringView node_name = node->GetTypeId().GetName();
            ZoneText(node_name.Data(), node_name.ByteLen());
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
                        if (const std::shared_ptr<asset::AssetBase> new_asset = factory->CreateAsset(node, context))
                        {
                            created_assets_map.Insert(node->GetUid(), new_asset);

                            // TODO: node->IsRoot() 같은거 추가해서 루트노드 검사

                            // ImportResult에 등록 (노드 이름을 Sub-Asset 이름으로 사용)
                            const String& display_name = node->GetDisplayName();
                            result_builder.RegisterAsset(new_asset, display_name);
                        }
                        return;
                    }
                }
                ConsoleLog(ELogLevel::Warning, "No factory found for node type: {}", node->GetTypeId().GetName());
            }();
        }
    }

    ImportResult result = result_builder.Build();
    if (result.IsEmpty())
    {
        return Unexpected(ImportError{
            ImportError::ECode::FactoryFailed,
            "No assets were created from the file",
            file_path
        });
    }

    return result;
}

Optional<IPipelineTranslator&> AssetImporter::FindTranslator(const Path& file_path) const
{
    const Optional ext_opt = file_path.Extension();
    if (!ext_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "Cannot find translator: file path has no extension: {}", file_path);
        return NullOpt;
    }

    const String ext = ext_opt->ToLower();
    for (const auto& translator : translators)
    {
        if (translator->CanTranslate(ext))
        {
            return *translator;
        }
    }
    return NullOpt;
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
        return {};
    }

    return result;
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
