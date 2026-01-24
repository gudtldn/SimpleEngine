#pragma once
#include <filesystem>
#include <memory>

#include "SimpleEngine/Asset/Pipeline/PipelineNodeContainer.h"
#include "SimpleEngine/Asset/Pipeline/PipelineProcessorStack.h"
#include "SimpleEngine/Asset/Pipeline/Factories/IPipelineFactory.h"
#include "SimpleEngine/Asset/Pipeline/Translators/IPipelineTranslator.h"
#include "SimpleEngine/Asset/Types/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API AssetImporter
{
public:
    AssetImporter() = default;
    ~AssetImporter() = default;

    // 복사만 금지
    AssetImporter(const AssetImporter&) = delete;
    AssetImporter& operator=(const AssetImporter&) = delete;
    AssetImporter(AssetImporter&&) = default;
    AssetImporter& operator=(AssetImporter&&) = default;

public:
    /** Asset Import시 사용할 Translator를 등록합니다. */
    template <typename Translator, typename... Args>
        requires std::derived_from<Translator, IPipelineTranslator>
    void RegisterTranslator(Args&&... args);

    /** Asset Import시 사용할 Factory를 등록합니다. */
    template <typename Factory, typename... Args>
        requires std::derived_from<Factory, IPipelineFactory>
    void RegisterFactory(Args&&... args);

    /**
     * 파일을 불러와 Asset 목록을 생성하여 반환합니다.
     * @param file_path 소스 파일 경로
     * @param import_config Import 설정
     * @param pipeline_stack 파이프라인 처리 스택 (선택)
     * @return 생성된 에셋 목록
     */
    [[nodiscard]] Array<std::shared_ptr<IAsset>> Import(
        const std::filesystem::path& file_path,
        const ImportConfig& import_config = {},
        Optional<const PipelineProcessorStack&> pipeline_stack = std::nullopt
    );

private:
    /** 파일 확장자에 맞는 Translator를 찾습니다. */ // TODO: 나중에 우선순위 같은거 정해야 할듯
    [[nodiscard]] Optional<IPipelineTranslator&> FindTranslator(const std::filesystem::path& file_path) const;

    /** 노드 간 의존성을 분석하여 생성 순서대로 위상 정렬합니다. */
    [[nodiscard]] static Array<PipelineBaseNode*> SortNodesByDependency(const PipelineNodeContainer& container);

private:
    Array<std::unique_ptr<IPipelineTranslator>> translators;
    Array<std::unique_ptr<IPipelineFactory>> factories;
};

template <typename Translator, typename ... Args>
    requires std::derived_from<Translator, IPipelineTranslator>
void AssetImporter::RegisterTranslator(Args&&... args)
{
    translators.Push(std::make_unique<Translator>(std::forward<Args>(args)...));
}

template <typename Factory, typename ... Args>
    requires std::derived_from<Factory, IPipelineFactory>
void AssetImporter::RegisterFactory(Args&&... args)
{
    factories.Push(std::make_unique<Factory>(std::forward<Args>(args)...));
}
}  // namespace se::asset
