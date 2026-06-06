#pragma once

#include "SimpleEditor/Asset/ImportProfile.h"
#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"

#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se::editor
{
/**
 * .meta 파일의 전체 내용을 표현하는 구조체
 * Core의 AssetMetadata(DTO)와 Editor의 ImportProfile을 조합하여, TOML .meta 파일로 직렬화합니다.
 */
struct SE_ANNOTATION(=meta::Reflect, =meta::Hidden) MetaFileContent
{
    /** 소스 파일의 메타데이터 (GUID, 해시, 수정시간 등) */
    SE_ANNOTATION(=meta::Reflect)
    AssetMetadata metadata;

    /** 임포트 설정 (Translator별 설정값) */
    SE_ANNOTATION(=meta::Reflect)
    ImportProfile import_settings;

    /** 파이프라인 Processor 실행 목록 (순서 보존) */
    SE_ANNOTATION(=meta::Reflect)
    Array<ProcessorEntry> processor_stack;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::MetaFileContent)
