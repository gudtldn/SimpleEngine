#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Asset/MetaFileContent.h"

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
/**
 * .meta 파일의 읽기/쓰기를 담당하는 정적 유틸리티 클래스
 *
 * 모든 .meta 파일은 TOML 포맷으로 저장되며,
 * MetaFileContent 구조체를 통해 직렬화/역직렬화됩니다.
 *
 * .meta 파일 경로 규칙: source_path + ".meta"
 *   예: "Assets/Models/character.fbx" -> "Assets/Models/character.fbx.meta"
 */
namespace asset_meta
{
/**
 * .meta 파일을 읽어 MetaFileContent로 반환합니다.
 * @param source_path 소스 파일 경로 (예: "Assets/character.fbx")
 * @return 성공 시 MetaFileContent, 실패 시 NullOpt
 */
[[nodiscard]] SE_EDITOR_API Optional<MetaFileContent> Load(const Path& source_path);

/**
 * MetaFileContent를 .meta 파일에 저장합니다.
 * .tmp 확장자로 임시 파일에 먼저 쓴 뒤, 원자적 Rename으로 교체합니다.
 * @param source_path 소스 파일 경로
 * @param content 저장할 내용
 * @return 저장 성공 여부
 */
SE_EDITOR_API bool Save(const Path& source_path, const MetaFileContent& content);

/**
 * 소스 파일에 대응하는 .meta 파일이 존재하는지 확인합니다.
 * @param source_path 소스 파일 경로
 * @return .meta 파일 존재 여부
 */
[[nodiscard]] SE_EDITOR_API bool Exists(const Path& source_path);

/**
 * 소스 파일에 대응하는 .meta 파일을 삭제합니다.
 * @param source_path 소스 파일 경로
 */
SE_EDITOR_API void Delete(const Path& source_path);

/**
 * 소스 파일 경로로부터 .meta 파일 경로를 계산합니다.
 * @param source_path 소스 파일 경로
 * @return .meta 파일 경로 (source_path + ".meta")
 */
[[nodiscard]] SE_EDITOR_API Path MetaPathOf(const Path& source_path);

/**
 * .meta 파일 경로에서 원본 소스 파일 경로를 역산합니다.
 * @param meta_path .meta 파일 경로 (예: "Assets/Models/character.fbx.meta")
 * @return 소스 파일 경로 (예: "Assets/Models/character.fbx")
 */
[[nodiscard]] SE_EDITOR_API Path SourcePathOf(const Path& meta_path);
} // namespace asset_meta
} // namespace se::editor
