// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/AssetImporter.h"

#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
bool AssetImporter::CanImport(const Path& file_path) const
{
    return FindTranslator(file_path).HasValue();
}

Optional<TypeId> AssetImporter::FindTranslatorTypeId(const Path& file_path) const
{
    if (const auto ext = file_path.Extension())
    {
        const String ext_lower = ext->ToLower();
        if (const auto indices_opt = extension_to_translator_indices.Find(ext_lower))
        {
            for (const usize idx : *indices_opt)
            {
                const TranslatorEntry& entry = translators[idx];
                if (entry.translator->CanTranslate(ext_lower))
                {
                    return entry.type_id;
                }
            }
        }
    }
    return NullOpt;
}

HashSet<StringView> AssetImporter::GetAllSupportedExtensions() const
{
    HashSet<StringView> result;
    for (const auto& [_, translator] : translators)
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
    ImportContext& io_ctx,
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
    const auto translator_opt = FindTranslator(file_path);
    if (!translator_opt)
    {
        ConsoleLog(ELogLevel::Error, "No suitable translator found for file: {}", file_path);
        return Unexpected<ImportError>({
            ImportError::NoTranslator,
            "No suitable translator found",
            file_path
        });
    }

    PipelineNodeContainer container;
    {
        // 각 노드에서 io_ctx.AllocateSubAssetGuid()로 UID를 발급
        ZoneScopedN("Translator::Translate");
        translator_opt->Translate(file_path, import_profile, io_ctx, container);
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
        return Unexpected<ImportError>({
            ImportError::CyclicDependency,
            "Cyclic dependency detected in pipeline nodes",
            file_path
        });
    }

    // ---------------------------------------------------------
    // 4단계: Factory 실행 (Nodes -> Assets)
    // ---------------------------------------------------------
    const PipelineImportContext factory_ctx = {
        .container = container,
        .registry = io_ctx.registry,
    };

    ImportResult::Builder result_builder;
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
                        if (std::shared_ptr<AssetBase> new_asset = factory->CreateAsset(node, factory_ctx))
                        {
                            // ImportResult에 등록 (노드 이름을 Sub-Asset 이름으로 사용)
                            result_builder.RegisterAsset(node->GetDisplayName(), AssetId{ node->GetUid() }, std::move(new_asset));
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
        return Unexpected<ImportError>({
            ImportError::FactoryFailed,
            "No assets were created from the file",
            file_path
        });
    }

    return result;
}

Optional<IPipelineTranslator&> AssetImporter::FindTranslator(const Path& file_path) const
{
    const auto ext_opt = file_path.Extension();
    if (!ext_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Warning, "Cannot find translator: file path has no extension: {}", file_path);
        return NullOpt;
    }

    const String ext_lower = ext_opt->ToLower();
    for (const auto& [_, translator] : translators)
    {
        if (translator->CanTranslate(ext_lower))
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
    HashMap<Guid, i32> in_degrees;           // 남은 의존성 개수

    // 그래프 구축 및 진입 차수(In-Degree) 계산
    Queue<PipelineBaseNode*> ready_queue;
    for (const auto& [node_id, node] : all_nodes)
    {
        // 현재 노드가 필요로 하는 의존성을 가져옴
        Array<Guid> dependencies;
        node->GetFactoryDependencies(dependencies);

        i32 current_in_degree = 0;
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
    while (auto node_opt = ready_queue.Pop())
    {
        PipelineBaseNode* current_node = *node_opt;
        result.Push(current_node);

        // current_node를 기다리던 다른 노드들을 확인
        if (const auto dependents = dependents_map.Find(current_node->GetUid()))
        {
            for (const Guid& dependent_id : *dependents)
            {
                // 해당 노드의 남은 의존성 개수 감소
                i32& degree = in_degrees[dependent_id];
                degree -= 1;

                // 모든 의존성이 해결되었다면 queue에 추가
                if (degree == 0)
                {
                    if (const auto next_node = container.GetNode(dependent_id))
                    {
                        ready_queue.Push(&next_node.Value());
                    }
                }
            }
        }
    }

    // 순환 의존성(Cycle) 감지
    if (result.Len() != all_nodes.Len())
    {
        ConsoleLog(ELogLevel::Error, "Cyclic dependency detected! Sorted {} out of {} nodes.", result.Len(), all_nodes.Len());
        for (const auto& [node_id, degree] : in_degrees)
        {
            if (degree > 0)
            {
                if (const auto node = container.GetNode(node_id))
                {
                    ConsoleLog(ELogLevel::Error, "- {} (Type: {}, Remaining Deps: {})",
                        node->GetDisplayName(), node->GetTypeId().GetName(), degree);
                }
            }
        }
        return {};
    }

    return result;
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
