#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/ImportProfile.h"
#include "SimpleEditor/Asset/Pipeline/ImportContext.h"
#include "SimpleEditor/Asset/Pipeline/ImportResult.h"
#include "SimpleEditor/Asset/Pipeline/Factories/IPipelineFactory.h"
#include "SimpleEditor/Asset/Pipeline/Translators/IPipelineTranslator.h"
#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"
#include "SimpleEditor/Asset/Pipeline/PipelineProcessorStack.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Path.h"

#include <memory>


namespace se::editor
{
/**
 * 파일을 Asset으로 변환하는 Import Pipeline
 *
 * 구조: File -> Translator -> PipelineNodes -> ProcessorStack -> Factory -> Assets
 */
class SE_EDITOR_API AssetImporter
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
     * 파일을 Import할 수 있는 Translator가 등록되어 있는지 확인합니다.
     * @param file_path Import가 가능한지 확인할 파일 경로 (확장자가 아님에 주의!)
     * @return 가능 여부
     */
    [[nodiscard]] bool CanImport(const Path& file_path) const;

    /**
     * 파일에 대응하는 Translator의 TypeId를 반환합니다.
     * @param file_path 소스 파일 경로
     * @return Translator의 TypeId, 없으면 NullOpt
     */
    [[nodiscard]] Optional<TypeId> FindTranslatorTypeId(const Path& file_path) const;

    /** 등록된 모든 Translator가 지원하는 확장자를 반환합니다. */
    [[nodiscard]] HashSet<StringView> GetAllSupportedExtensions() const;

    /**
     * 파일을 불러와 ImportResult를 반환합니다. (우선 Main Thread 전용)
     * @param file_path 소스 파일 경로
     * @param io_ctx sub-asset GUID 발급 및 cross-file 참조 해소 컨텍스트
     * @param import_profile Import 설정
     * @param processor_stack 파이프라인 처리 스택 (선택)
     * @return 성공 시 ImportResult, 실패 시 ImportError
     */
    [[nodiscard]] Expected<ImportResult, ImportError> Import(
        const Path& file_path,
        ImportContext& io_ctx,
        const ImportProfile& import_profile = {},
        Optional<const PipelineProcessorStack&> processor_stack = NullOpt
    );

private:
    /** 파일 확장자에 맞는 Translator를 찾습니다. */ // TODO: 나중에 우선순위 같은거 정해야 할듯
    [[nodiscard]] Optional<IPipelineTranslator&> FindTranslator(const Path& file_path) const;

    /** 노드 간 의존성을 분석하여 생성 순서대로 위상 정렬합니다. */
    [[nodiscard]] static Array<PipelineBaseNode*> SortNodesByDependency(const PipelineNodeContainer& container);

private:
    struct TranslatorEntry
    {
        /** Translator의 TypeId */
        TypeId type_id;

        /** 실제 Translator의 Instance */
        std::unique_ptr<IPipelineTranslator> translator;
    };

    Array<TranslatorEntry> translators;
    HashMap<String, Array<usize>> extension_to_translator_indices;

    Array<std::unique_ptr<IPipelineFactory>> factories;
};

template <typename Translator, typename... Args>
    requires std::derived_from<Translator, IPipelineTranslator>
void AssetImporter::RegisterTranslator(Args&&... args)
{
    const usize new_index = translators.Len();
    auto instance = std::make_unique<Translator>(std::forward<Args>(args)...);

    for (const StringView& ext : instance->GetSupportedExtensions())
    {
        String lower_ext = String{ ext }.ToLower();
        extension_to_translator_indices.Entry(std::move(lower_ext)).OrDefault().Push(new_index);
    }

    translators.Push({
        .type_id = TypeId::Get<Translator>(),
        .translator = std::move(instance),
    });
}

template <typename Factory, typename... Args>
    requires std::derived_from<Factory, IPipelineFactory>
void AssetImporter::RegisterFactory(Args&&... args)
{
    factories.Push(std::make_unique<Factory>(std::forward<Args>(args)...));
}
} // namespace se::editor
