#pragma once
#include <memory>

#include "SimpleEngine/Asset/Pipeline/ImportResult.h"
#include "SimpleEngine/Asset/Pipeline/PipelineNodeContainer.h"
#include "SimpleEngine/Asset/Pipeline/PipelineProcessorStack.h"
#include "SimpleEngine/Asset/Pipeline/Factories/IPipelineFactory.h"
#include "SimpleEngine/Asset/Pipeline/Translators/IPipelineTranslator.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::asset
{
/**
 * 파일을 Asset으로 변환하는 Import Pipeline 클래스
 *
 * 구조: File -> Translator -> PipelineNodes -> ProcessorStack -> Factory -> Assets
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
     * 파일을 불러와 ImportResult를 반환합니다.
     * @param file_path 소스 파일 경로
     * @param import_config Import 설정
     * @param processor_stack 파이프라인 처리 스택 (선택)
     * @return 성공 시 ImportResult, 실패 시 ImportError
     */
    [[nodiscard]] Expected<ImportResult, ImportError> Import(
        const Path& file_path,
        const ImportConfig& import_config = {},
        Optional<const PipelineProcessorStack&> processor_stack = std::nullopt
    );

private:
    /** 파일 확장자에 맞는 Translator를 찾습니다. */ // TODO: 나중에 우선순위 같은거 정해야 할듯
    [[nodiscard]] Optional<IPipelineTranslator&> FindTranslator(const Path& file_path) const;

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
