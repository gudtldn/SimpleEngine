// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/v2/AssetImporter.h"

#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "tracy/Tracy.hpp"


namespace se::editor::v2
{
bool AssetImporter::CanImport(const Path& file_path) const
{
    return FindTranslator(file_path).HasValue();
}

Optional<TypeId> AssetImporter::FindTranslatorTypeId(const Path& file_path) const
{
    if (const Optional ext_opt = file_path.Extension())
    {
        const String ext = ext_opt->ToLower();
        if (const auto indices_opt = extension_to_translator_indices.Find(ext))
        {
            for (const usize idx : *indices_opt)
            {
                const TranslatorEntry& entry = translators[idx];
                if (entry.translator->CanTranslate(ext))
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

    // 1. Translator 실행 — 각 노드가 io_ctx.AllocateSubAssetGuid()로 UID를 발급받음
    const Optional translator_opt = FindTranslator(file_path);
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
        ZoneScopedN("Translator::Translate");
        translator_opt->Translate(file_path, import_profile, io_ctx, container);
    }

    // 2. Pipeline Stack 실행
    if (processor_stack)
    {
        ZoneScopedN("ProcessorStack::ExecuteStack");
        processor_stack->ExecuteStack(container);
    }

    // 3. 의존성 정렬
    const Array<PipelineBaseNode*> sorted_nodes = [&container]
    {
        ZoneScopedN("SortNodesByDependency"); // NOLINT(*-lambda-function-name)
        return SortNodesByDependency(container);
    }();

    if (sorted_nodes.IsEmpty() && !container.GetAllNodes().IsEmpty())
    {
        return Unexpected<ImportError>({
            ImportError::CyclicDependency,
            "Cyclic dependency detected in pipeline nodes",
            file_path
        });
    }

    // 4. Factory 실행 — node->GetUid() == AssetId.guid 불변식으로 created_assets_map 불필요
    const PipelineImportContext factory_ctx{
        .container = container,
        .registry  = io_ctx.registry,
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
                for (const auto& factory : factories)
                {
                    if (factory->CanCreateAsset(node))
                    {
                        ZoneScopedN("Factory::CreateAsset"); // NOLINT(*-lambda-function-name)
                        if (std::shared_ptr<AssetBase> new_asset = factory->CreateAsset(node, factory_ctx))
                        {
                            result_builder.RegisterAsset(
                                node->GetDisplayName(),
                                AssetId{ node->GetUid() },
                                std::move(new_asset)
                            );
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
    // container 소멸 <- PipelineNodeContainer RAII 해제 (ADR-002)
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
    for (const auto& [_, translator] : translators)
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

    HashMap<Guid, Array<Guid>> dependents_map;
    HashMap<Guid, int32> in_degrees;

    Queue<PipelineBaseNode*> ready_queue;
    for (const auto& [node_id, node] : all_nodes)
    {
        Array<Guid> dependencies;
        node->GetFactoryDependencies(dependencies);

        int32 current_in_degree = 0;
        for (const Guid& dependency_id : dependencies)
        {
            if (container.Contains(dependency_id))
            {
                dependents_map[dependency_id].Push(node_id);
                ++current_in_degree;
            }
        }

        in_degrees.Insert(node_id, current_in_degree);

        if (current_in_degree == 0)
        {
            ready_queue.Push(node.get());
        }
    }

    while (Optional node_opt = ready_queue.Pop())
    {
        PipelineBaseNode* current_node = *node_opt;
        result.Push(current_node);

        if (Optional dependents_opt = dependents_map.Find(current_node->GetUid()))
        {
            for (const Guid& dependent_id : *dependents_opt)
            {
                int32& degree = in_degrees[dependent_id];
                degree -= 1;

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

    if (result.Len() != all_nodes.Len())
    {
        ConsoleLog(ELogLevel::Error, "Cyclic dependency detected! Sorted {} out of {} nodes.", result.Len(), all_nodes.Len());
        for (const auto& [node_id, degree] : in_degrees)
        {
            if (degree > 0)
            {
                if (const Optional node_opt = container.GetNode(node_id))
                {
                    ConsoleLog(ELogLevel::Error, "- {} (Type: {}, Remaining Deps: {})",
                        node_opt->GetDisplayName(), node_opt->GetTypeId().GetName(), degree);
                }
            }
        }
        return {};
    }

    return result;
}
} // namespace se::editor::v2
// NOLINTEND(*-reserved-identifier)
