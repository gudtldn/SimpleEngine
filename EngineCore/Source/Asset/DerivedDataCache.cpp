// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Asset/DerivedDataCache.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"

#include "tracy/Tracy.hpp"


namespace se::asset
{
/** 캐시 파일 매직 넘버 ("SEDC" = SimpleEngine Derived Cache) */
static constexpr uint32 CACHE_MAGIC =
    static_cast<uint32>('S')
    | (static_cast<uint32>('E') << 8)
    | (static_cast<uint32>('D') << 16)
    | (static_cast<uint32>('C') << 24);

/** 캐시 파일 포맷 버전 */
static constexpr uint32 CACHE_FORMAT_VERSION = 1;

/** 캐시 파일 확장자 */
static constexpr StringView CACHE_EXTENSION = ".cache";

/** 임시 파일 확장자 (atomic write용) */
static constexpr StringView TEMP_EXTENSION = ".cache.tmp";


DerivedDataCache::DerivedDataCache(Path in_root_path)
    : root_path(std::move(in_root_path))
{
    // DDC 루트 디렉토리가 없으면 생성
    if (!root_path.Exists())
    {
        FileSystem::CreateDirectories(root_path);
    }
}

bool DerivedDataCache::Store(const Guid& guid, CacheEntry&& entry)
{
    ZoneScopedN("DDC::Store");

    const Path cache_path = BuildCachePath(guid);
    const Path temp_path = BuildTempPath(guid);

    // 버킷 디렉토리 생성
    if (const Optional parent = cache_path.Parent())
    {
        if (!parent->Exists())
        {
            FileSystem::CreateDirectories(*parent);
        }
    }

    // MemoryWriter로 캐시 데이터 직렬화
    Array<uint8> buffer;
    MemoryWriter writer(buffer);

    uint32 magic = CACHE_MAGIC;
    uint32 format_ver = CACHE_FORMAT_VERSION;

    // 기본 Header 직렬화
    writer << magic
           << format_ver
           << entry.source_hash
           << entry.cache_version;

    // Payload 직렬화
    writer << entry.payload;

    // Atomic Write: 임시 파일에 먼저 쓰고 rename
    if (!FileSystem::Write(temp_path, buffer))
    {
        ConsoleLog(ELogLevel::Error, "DDC::Store - Failed to write temp file: {}", temp_path);
        return false;
    }

    // 기존 캐시 파일이 있으면 먼저 삭제 (Rename이 대상 파일 존재 시 실패할 수 있음)
    if (cache_path.Exists())
    {
        FileSystem::Remove(cache_path);
    }

    if (!FileSystem::Rename(temp_path, cache_path))
    {
        ConsoleLog(ELogLevel::Error, "DDC::Store - Failed to rename temp -> cache: {} -> {}", temp_path, cache_path);
        FileSystem::Remove(temp_path);
        return false;
    }

    return true;
}

Optional<CacheEntry> DerivedDataCache::Load(const Guid& guid) const
{
    ZoneScopedN("DDC::Load");

    const Path cache_path = BuildCachePath(guid);

    const Optional buffer_opt = FileSystem::Read(cache_path);
    if (!buffer_opt.HasValue())
    {
        return std::nullopt;
    }

    MemoryReader reader(buffer_opt.Value());

    // Magic 검증
    uint32 magic = 0;
    reader << magic;
    if (reader.HasError() || magic != CACHE_MAGIC)
    {
        ConsoleLog(ELogLevel::Warning, "DDC::Load - Invalid magic in: {}", cache_path);
        return std::nullopt;
    }

    // Format Version 검증
    uint32 format_ver = 0;
    reader << format_ver;
    if (reader.HasError() || format_ver != CACHE_FORMAT_VERSION)
    {
        ConsoleLog(
            ELogLevel::Warning,
            "DDC::Load - Format version mismatch (expected: {}, got: {}): {}",
            CACHE_FORMAT_VERSION, format_ver, cache_path
        );
        return std::nullopt;
    }

    // Source Hash + Cache Version
    CacheEntry entry;
    reader << entry.source_hash;
    reader << entry.cache_version;

    // Payload 역직렬화
    reader << entry.payload;

    if (reader.HasError())
    {
        ConsoleLog(ELogLevel::Warning, "DDC::Load - Serialization error: {} in {}", reader.GetError(), cache_path);
        return std::nullopt;
    }

    return entry;
}

// source_hash와 cache_version이 모두 일치해야 유효한 것으로 판단.
bool DerivedDataCache::IsValid(
    const Guid& guid,
    StringView source_hash,
    uint32 cache_version
) const
{
    const Path cache_path = BuildCachePath(guid);

    String stored_hash;
    uint32 stored_version = 0;

    if (!ReadHeader(cache_path, stored_hash, stored_version))
    {
        return false;
    }

    return stored_hash == source_hash && stored_version == cache_version;
}

bool DerivedDataCache::Contains(const Guid& guid) const
{
    return BuildCachePath(guid).Exists();
}

bool DerivedDataCache::Remove(const Guid& guid)
{
    const Path cache_path = BuildCachePath(guid);

    if (!cache_path.Exists())
    {
        return true;
    }

    return FileSystem::Remove(cache_path);
}

void DerivedDataCache::Clear()
{
    ZoneScopedN("DDC::Clear");

    if (root_path.Exists())
    {
        const usize removed = FileSystem::RemoveAll(root_path);
        ConsoleLog(ELogLevel::Info, "DDC::Clear - Removed {} entries from: {}", removed, root_path);

        // 루트 디렉토리 재생성
        FileSystem::CreateDirectories(root_path);
    }
}

Path DerivedDataCache::BuildCachePath(const Guid& guid) const
{
    const String guid_str = guid.ToString();

    // 앞 2글자를 버킷 디렉토리로 사용 (예: "ab" / "abcdef01-...")
    const String bucket = guid_str.Substring(0, 2);
    const String filename = guid_str + String{ CACHE_EXTENSION };

    return root_path / Path{ bucket } / Path{ filename };
}

Path DerivedDataCache::BuildTempPath(const Guid& guid) const
{
    const String guid_str = guid.ToString();
    const String bucket = guid_str.Substring(0, 2);
    const String filename = guid_str + String{ TEMP_EXTENSION };

    return root_path / Path{ bucket } / Path{ filename };
}

bool DerivedDataCache::ReadHeader(
    const Path& cache_path,
    String& out_source_hash,
    uint32& out_cache_version
)
{
    const Optional buffer_opt = FileSystem::Read(cache_path);
    if (!buffer_opt.HasValue())
    {
        return false;
    }

    MemoryReader reader(buffer_opt.Value());

    // Magic + Format Version 검증
    uint32 magic = 0;
    reader << magic;
    if (reader.HasError() || magic != CACHE_MAGIC)
    {
        return false;
    }

    uint32 format_ver = 0;
    reader << format_ver;
    if (reader.HasError() || format_ver != CACHE_FORMAT_VERSION)
    {
        return false;
    }

    // Source Hash + Cache Version만 읽고 payload는 건너뜀
    reader << out_source_hash;
    reader << out_cache_version;

    return !reader.HasError();
}
}  // namespace se::asset
