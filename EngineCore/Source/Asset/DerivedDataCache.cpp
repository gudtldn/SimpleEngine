// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Asset/DerivedDataCache.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include "tracy/Tracy.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>


namespace se
{
namespace
{
/** 캐시 파일 확장자 */
constexpr StringView CACHE_EXTENSION = ".cache";

/** 임시 파일 확장자 (atomic write용) */
constexpr StringView TEMP_EXTENSION = ".cache.tmp";

/**
 * 캐시 파일 맨 앞에 두는 엔트리 정보. 구조체의 메모리 표현을 그대로 쓰고 읽으므로 패딩 없이 둡니다.
 * payload(에셋의 Packed 파일)와 달리 체크섬 밖에 있어, IsValid가 파일 앞부분만 읽고 판단할 수 있습니다.
 */
struct CachePrefix
{
    u32 cache_version = 0;
    ContentHash source_hash;
};
static_assert(std::has_unique_object_representations_v<CachePrefix>, "CachePrefix must not contain padding bytes.");

/** buffer 앞의 CachePrefix를 읽습니다. buffer가 CachePrefix보다 짧으면 NullOpt를 돌려줍니다. */
[[nodiscard]] Optional<CachePrefix> ReadPrefix(ArrayView<const u8> buffer)
{
    if (buffer.Len() < sizeof(CachePrefix))
    {
        return NullOpt;
    }

    CachePrefix prefix;
    std::memcpy(&prefix, buffer.Data(), sizeof(prefix));
    return prefix;
}

/** 캐시 파일의 앞부분만 읽어 CachePrefix를 얻습니다. 읽지 못했거나 파일이 짧으면 NullOpt를 돌려줍니다. */
[[nodiscard]] Optional<CachePrefix> ReadPrefixFromFile(const Path& cache_path)
{
    static constexpr usize CHUNK_SIZE = 128;
    static_assert(sizeof(CachePrefix) <= CHUNK_SIZE);

    for (auto&& file_result : fs::ReadChunked(cache_path, CHUNK_SIZE))
    {
        if (file_result.HasError())
        {
            ConsoleLog(ELogLevel::Warning, "DDC::ReadPrefix - IO Error: {}, {}", cache_path, file_result.Error().What());
            return NullOpt;
        }

        if (const auto prefix = ReadPrefix(*file_result))
        {
            return prefix;
        }
        break;
    }

    ConsoleLog(ELogLevel::Warning, "DDC::ReadPrefix - File is too short for the entry prefix: {}", cache_path);
    return NullOpt;
}
} // namespace


DerivedDataCache::DerivedDataCache(Path in_root_path)
    : root_path(std::move(in_root_path))
{
    // DDC 루트 디렉토리가 없으면 생성
    if (!root_path.Exists())
    {
        fs::CreateDirectories(root_path);
    }
}

Optional<CacheEntry> DerivedDataCache::ParseFromBuffer(ArrayView<const u8> buffer_view)
{
    const auto prefix = ReadPrefix(buffer_view);
    if (!prefix)
    {
        ConsoleLog(ELogLevel::Warning, "DDC::ParseFromBuffer - {} bytes is too short for the entry prefix", buffer_view.Len());
        return NullOpt;
    }

    const ArrayView<const u8> payload = buffer_view.Subview(sizeof(CachePrefix));
    return CacheEntry{
        .source_hash = prefix->source_hash,
        .cache_version = prefix->cache_version,
        .payload = Array<u8>(payload.begin(), payload.end()),
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
            fs::CreateDirectories(*parent);
        }
    }

    // CachePrefix 뒤에 payload를 이어 붙임
    const CachePrefix prefix{
        .cache_version = entry.cache_version,
        .source_hash = entry.source_hash,
    };
    Array<u8> buffer(sizeof(CachePrefix) + entry.payload.Len());
    std::memcpy(buffer.Data(), &prefix, sizeof(prefix));
    std::ranges::copy(entry.payload, buffer.Data() + sizeof(prefix));

    // Atomic Write: 임시 파일에 먼저 쓰고 rename
    if (!fs::Write(temp_path, buffer))
    {
        ConsoleLog(ELogLevel::Error, "DDC::Store - Failed to write temp file: {}", temp_path);
        return false;
    }

    if (!fs::Rename(temp_path, cache_path))
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

    const FileResult<Array<u8>> buffer_result = fs::ReadBytes(cache_path);
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
    const auto stored_prefix = ReadPrefixFromFile(BuildCachePath(guid));
    if (!stored_prefix)
    {
        return false;
    }

    return stored_prefix->source_hash == source_hash
        && stored_prefix->cache_version == cache_version;
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

    return fs::Remove(cache_path);
}

void DerivedDataCache::Clear()
{
    ZoneScopedN("DDC::Clear");

    if (root_path.Exists())
    {
        const usize removed = fs::RemoveAll(root_path);
        ConsoleLog(ELogLevel::Info, "DDC::Clear - Removed {} entries from: {}", removed, root_path);

        // 루트 디렉토리 재생성
        fs::CreateDirectories(root_path);
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
