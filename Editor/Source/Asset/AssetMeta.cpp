#include "SimpleEditor/Asset/AssetMeta.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/Legacy/TomlArchive.h"

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
    const usize thread_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    return String::Format("{}_{}{}", meta_path, thread_hash, TEMP_EXTENSION);
}
} // namespace

namespace asset_meta
{
Optional<MetaFileContent> Load(const Path& source_path)
{
    ZoneScopedN("asset_meta::Load");

    const Path meta_path = MetaPathOf(source_path);
    if (!meta_path.Exists())
    {
        return NullOpt;
    }

    // TOML 파일 읽기
    const auto file_content = fs::ReadToString(meta_path);
    if (!file_content.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Failed to read meta file: {}", meta_path);
        return NullOpt;
    }

    // TOML 파싱
    const toml::parse_result parse_result = toml::parse(file_content.Value().Bytes());
    if (!parse_result)
    {
        ConsoleLog(ELogLevel::Error, "Failed to parse meta file: {} - {}", meta_path, parse_result.error().description());
        return NullOpt;
    }

    // 역직렬화
    MetaFileContent content;
    TomlReader_v1 reader(parse_result.table());
    reader << content;

    return content;
}

bool Save(const Path& source_path, const MetaFileContent& content)
{
    ZoneScopedN("asset_meta::Save");

    const Path meta_path = MetaPathOf(source_path);

    // 부모 디렉토리 보장
    if (const auto parent = meta_path.Parent())
    {
        if (!parent->Exists())
        {
            fs::CreateDirectories(*parent);
        }
    }

    // TOML 트리에 직렬화
    toml::table root;
    TomlWriter_v1 writer(root);
    writer << content;

    // TOML 문자열 생성
    std::ostringstream oss;
    oss << root;

    // Atomic Write: .tmp에 먼저 쓰고 rename
    const Path temp_path = BuildTempPath(meta_path);
    SE_SCOPE_DEFER_NAMED(rollback) {
        fs::Remove(temp_path);
    };

    if (!fs::WriteString(temp_path, oss.view()))
    {
        ConsoleLog(ELogLevel::Error, "asset_meta::Save - Failed to write temp file: {}", temp_path.ToString());
        return false;
    }

    if (!fs::Rename(temp_path, meta_path))
    {
        ConsoleLog(ELogLevel::Error, "asset_meta::Save - Failed to rename temp -> meta: {} -> {}", temp_path.ToString(), meta_path.ToString());
        return false;
    }

    rollback.Discard();
    return true;
}

bool Exists(const Path& source_path)
{
    return MetaPathOf(source_path).Exists();
}

void Delete(const Path& source_path)
{
    const Path meta_path = MetaPathOf(source_path);
    if (meta_path.Exists())
    {
        if (!fs::Remove(meta_path))
        {
            ConsoleLog(ELogLevel::Warning, "Failed to delete meta file: {}", meta_path.ToString());
        }
    }
}

Path MetaPathOf(const Path& source_path)
{
    Path meta_path = source_path;
    meta_path += META_EXTENSION;
    return meta_path;
}

Path SourcePathOf(const Path& meta_path)
{
    // "dir/foo.fbx.meta" -> FileStem()="foo.fbx", Parent()="dir" -> "dir/foo.fbx"
    const String stem = meta_path.FileStem().ValueOrDefault();
    if (const auto parent = meta_path.Parent())
    {
        return *parent / stem;
    }
    return stem;
}
} // namespace asset_meta
} // namespace se::editor
