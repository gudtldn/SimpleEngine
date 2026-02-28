#include "SimpleEditor/Asset/MetaFileManager.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "tracy/Tracy.hpp"

#include <sstream>
#include <thread>


namespace se::editor
{
namespace
{
constexpr StringView META_EXTENSION = ".meta";
constexpr StringView TEMP_EXTENSION = ".tmp";

/**
 * 임시 파일 경로를 생성합니다. (Atomic Write용)
 * 스레드 ID 해시를 포함하여 멀티스레드 환경에서도 충돌을 방지합니다.
 */
Path BuildTempPath(const Path& meta_path)
{
    const size_t thread_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    return String::Format("_{}_{}{}", thread_hash, meta_path, TEMP_EXTENSION);
}
} // namespace

Optional<MetaFileContent> MetaFileManager::Load(const Path& source_path)
{
    ZoneScopedN("MetaFileManager::Load");

    const Path meta_path = GetMetaPath(source_path);
    if (!meta_path.Exists())
    {
        ConsoleLog(ELogLevel::Warning, "Meta file not found: {}", meta_path.ToString());
        return NullOpt;
    }

    // TOML 파일 읽기
    const auto file_content = FileSystem::ReadToString(meta_path);
    if (!file_content.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Failed to read meta file: {}", meta_path.ToString());
        return NullOpt;
    }

    // TOML 파싱
    const toml::parse_result parse_result = toml::parse(file_content.Value().CStr());
    if (!parse_result)
    {
        ConsoleLog(ELogLevel::Error, "Failed to parse meta file: {} - {}",
            meta_path.ToString(), parse_result.error().description());
        return NullOpt;
    }

    // 역직렬화
    MetaFileContent content;
    TomlReader reader(parse_result.table());
    reader << content;

    return content;
}

bool MetaFileManager::Save(const Path& source_path, const MetaFileContent& content)
{
    ZoneScopedN("MetaFileManager::Save");

    const Path meta_path = GetMetaPath(source_path);

    // 부모 디렉토리 보장
    if (const Optional parent = meta_path.Parent())
    {
        if (!parent->Exists())
        {
            FileSystem::CreateDirectories(*parent);
        }
    }

    // TOML 트리에 직렬화
    toml::table root;
    TomlWriter writer(root);

    MetaFileContent mutable_content = content;
    writer << mutable_content;

    // TOML 문자열 생성
    std::ostringstream oss;
    oss << root;
    const String toml_string = StringUtils::ToString(oss.str());

    // Atomic Write: .tmp에 먼저 쓰고 rename
    const Path temp_path = BuildTempPath(meta_path);
    SE_SCOPE_DEFER_NAMED(rollback) {
        FileSystem::Remove(temp_path);
    };

    if (!FileSystem::WriteString(temp_path, toml_string))
    {
        ConsoleLog(ELogLevel::Error, "MetaFileManager::Save - Failed to write temp file: {}", temp_path.ToString());
        return false;
    }

    if (!FileSystem::Rename(temp_path, meta_path))
    {
        ConsoleLog(ELogLevel::Error, "MetaFileManager::Save - Failed to rename temp -> meta: {} -> {}", temp_path.ToString(), meta_path.ToString());
        return false;
    }

    rollback.Discard();
    return true;
}

bool MetaFileManager::HasMeta(const Path& source_path)
{
    return GetMetaPath(source_path).Exists();
}

void MetaFileManager::DeleteMeta(const Path& source_path)
{
    const Path meta_path = GetMetaPath(source_path);
    if (meta_path.Exists())
    {
        if (!FileSystem::Remove(meta_path))
        {
            ConsoleLog(ELogLevel::Warning, "Failed to delete meta file: {}", meta_path.ToString());
        }
    }
}

Path MetaFileManager::GetMetaPath(const Path& source_path)
{
    Path meta_path = source_path;
    meta_path += META_EXTENSION;
    return meta_path;
}
} // namespace se::editor
