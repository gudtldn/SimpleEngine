#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/ImportProfile.h"
#include "SimpleEditor/Asset/Pipeline/PipelineNodeContainer.h"

#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
/**
 * 소스 파일을 PipelineNode로 변환하는 인터페이스
 */
class SE_EDITOR_API IPipelineTranslator
{
public:
    virtual ~IPipelineTranslator() = default;

    /**
     * Translator가 지원하는 파일 확장자 목록을 반환합니다.
     * @return 지원하는 파일 확장자 목록
     */
    [[nodiscard]] virtual ArrayView<const StringView> GetSupportedExtensions() const = 0;

    /**
     * Translator가 지원하는 파일 확장자인지 확인합니다.
     * @param file_extension 파일 확장자 (ex: .obj)
     */
    [[nodiscard]] virtual bool CanTranslate(const String& file_extension) const
    {
        return std::ranges::any_of(GetSupportedExtensions(), [&](const StringView& ext)
        {
            return ext == file_extension;
        });
    }

    /**
     * 파일을 읽고 Container에 Node를 채웁니다.
     * @param file_path 파일 경로
     * @param import_profile 파일을 가져올 때 사용할 ImportProfile
     * @param out_container 변환된 노드들을 담을 컨테이너
     */
    virtual void Translate(
        const Path& file_path,
        const ImportProfile& import_profile,
        PipelineNodeContainer& out_container
    ) = 0;
};
} // namespace se::editor
