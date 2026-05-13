// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Asset/DerivedDataCache.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"

#include "tracy/Tracy.hpp"


namespace se
{
namespace
{
/** 캐시 파일 매직 넘버 ("SEDC" = SimpleEngine Derived Cache) */
constexpr u32 CACHE_MAGIC =
    static_cast<u32>('S')
    | (static_cast<u32>('E') << 8)
    | (static_cast<u32>('D') << 16)
    | (static_cast<u32>('C') << 24);

/** 캐시 파일 포맷 버전 */
constexpr u32 CACHE_FORMAT_VERSION = 1;

/** 캐시 파일 확장자 */
constexpr StringView CACHE_EXTENSION = ".cache";

/** 임시 파일 확장자 (atomic write용) */
constexpr StringView TEMP_EXTENSION = ".cache.tmp";

/**
 * DDC 캐시 파일에서 읽어온 엔트리 정보 (Internal)
 */
struct DDC_CacheEntryInternal
{
    struct Header
    {
        u32 magic = CACHE_MAGIC;
        u32 format_version = CACHE_FORMAT_VERSION;
        u32 cache_version = 0;
        ContentHash source_hash;

        friend void Serialize(Archive& ar, Header& ar_header)
        {
            ar("magic") << ar_header.magic;
            ar("format_version") << ar_header.format_version;
            ar("cache_version") << ar_header.cache_version;
            ar("source_hash") << ar_header.source_hash;
        }
    } header;

    Array<u8> payload;

    friend void Serialize(Archive& ar, DDC_CacheEntryInternal& entry)
    {
        ar("header") << entry.header;
        ar("payload") << entry.payload;
    }
};

bool ReadHeader(
    const Path& cache_path,
    DDC_CacheEntryInternal::Header& out_header
)
{
    static constexpr usize CHUNK_SIZE = 128;
    static_assert(sizeof(DDC_CacheEntryInternal::Header) <= CHUNK_SIZE);

    DDC_CacheEntryInternal::Header header;
    bool deserialize_success = false;

    for (auto&& file_result : FileSystem::ReadChunked(cache_path, CHUNK_SIZE))
    {
        // 파일 시스템 에러 체크
        if (file_result.HasError())
        {
            ConsoleLog(ELogLevel::Warning, "DDC::ReadHeader - IO Error: {}, {}", cache_path, file_result.Error().What());
            return false;
        }

        MemoryReader reader{ *file_result };
        reader << header;

        deserialize_success = !reader.HasError();
        break;
    }

    // 역직렬화 실패 체크
    if (!deserialize_success)
    {
        ConsoleLog(ELogLevel::Warning, "DDC::ReadHeader - Deserialize failed: {}", cache_path);
        return false;
    }

    // Magic + Format Version 검증
    if (header.magic != CACHE_MAGIC || header.format_version != CACHE_FORMAT_VERSION)
    {
        ConsoleLog(
            ELogLevel::Warning,
            "DDC::ReadHeader - Invalid Format (Magic: {:#x}, Ver: {}): {}",
            header.magic, header.format_version, cache_path
        );
        return false;
    }

    out_header = std::move(header);
    return deserialize_success;
}
} // namespace


DerivedDataCache::DerivedDataCache(Path in_root_path)
    : root_path(std::move(in_root_path))
{
    // DDC 루트 디렉토리가 없으면 생성
    if (!root_path.Exists())
    {
        FileSystem::CreateDirectories(root_path);
    }
}

Optional<CacheEntry> DerivedDataCache::ParseFromBuffer(ArrayView<const u8> buffer_view)
{
    MemoryReader reader{ buffer_view };
    DDC_CacheEntryInternal cache_internal;

    // Header 역직렬화
    reader << cache_internal.header;

    if (reader.HasError())
    {
        ConsoleLog(ELogLevel::Warning, "DDC::ParseFromBuffer - Header deserialization failed");
        return NullOpt;
    }

    // Magic 검증
    if (cache_internal.header.magic != CACHE_MAGIC)
    {
        ConsoleLog(ELogLevel::Warning, "DDC::ParseFromBuffer - Invalid magic: {:#x}", cache_internal.header.magic);
        return NullOpt;
    }

    // Format Version 검증
    if (cache_internal.header.format_version != CACHE_FORMAT_VERSION)
    {
        ConsoleLog(
            ELogLevel::Warning,
            "DDC::ParseFromBuffer - Format version mismatch (expected: {}, got: {})",
            CACHE_FORMAT_VERSION, cache_internal.header.format_version
        );
        return NullOpt;
    }

    // Payload 역직렬화
    reader << cache_internal.payload;

    return CacheEntry{
        .source_hash = std::move(cache_internal.header.source_hash),
        .cache_version = cache_internal.header.cache_version,
        .payload = std::move(cache_internal.payload),
    };
}

bool DerivedDataCache::Store(const Guid& guid, CacheEntry&& entry)
{
    ZoneScopedN("DDC::Store");

    const Path cache_path = BuildCachePath(guid);
    const Path temp_path = BuildTempPath(guid);

    // 버킷 디렉토리 생성
    if (const auto parent = cache_path.Parent())
    {
        if (!parent->Exists())
        {
            FileSystem::CreateDirectories(*parent);
        }
    }

    // MemoryWriter로 캐시 데이터 직렬화
    Array<u8> buffer;
    MemoryWriter writer(buffer);

    DDC_CacheEntryInternal cache_internal;
    cache_internal.header = {
        .cache_version = entry.cache_version,
        .source_hash = std::move(entry.source_hash),
    };
    cache_internal.payload = std::move(entry.payload);

    // Cache Entry 직렬화
    writer << cache_internal;

    // Atomic Write: 임시 파일에 먼저 쓰고 rename
    if (!FileSystem::Write(temp_path, buffer))
    {
        ConsoleLog(ELogLevel::Error, "DDC::Store - Failed to write temp file: {}", temp_path);
        return false;
    }

    if (!FileSystem::Rename(temp_path, cache_path))
    {
        ConsoleLog(ELogLevel::Error, "DDC::Store - Failed to rename temp -> cache: {} -> {}", temp_path, cache_path);
        return false;
    }

    return true;
}

Optional<CacheEntry> DerivedDataCache::Load(const Guid& guid) const
{
    ZoneScopedN("DDC::Load");

    const Path cache_path = BuildCachePath(guid);

    const FileResult<Array<u8>> buffer_result = FileSystem::ReadBytes(cache_path);
    if (buffer_result)
    {
        return ParseFromBuffer(buffer_result.Value());
    }

    ConsoleLog(ELogLevel::Warning, "DDC::Load - Failed to read cache file: {}, Err: {}", cache_path, buffer_result.Error().What());
    return NullOpt;
}

// source_hash와 cache_version이 모두 일치해야 유효한 것으로 판단.
bool DerivedDataCache::IsValid(
    const Guid& guid,
    const ContentHash& source_hash,
    u32 cache_version
) const
{
    const Path cache_path = BuildCachePath(guid);

    DDC_CacheEntryInternal::Header stored_header;
    if (!ReadHeader(cache_path, stored_header))
    {
        return false;
    }

    return stored_header.source_hash == source_hash
        && stored_header.cache_version == cache_version;
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
    const String filename = String::Format("{}{}", guid_str, CACHE_EXTENSION);

    return root_path / bucket / filename;
}

Path DerivedDataCache::BuildTempPath(const Guid& guid) const
{
    const String guid_str = guid.ToString();
    const String bucket = guid_str.Substring(0, 2);

    const usize thread_id_hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    const String filename = String::Format("{}_{}{}", guid_str, thread_id_hash, TEMP_EXTENSION);

    return root_path / bucket / filename;
}
} // namespace se
