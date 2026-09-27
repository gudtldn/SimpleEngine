#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetPayload.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "TestAssetFactories.h"

#include "SDL3/SDL_filesystem.h"

#include <atomic>
#include <string_view>
#include <thread>

using namespace se;

namespace
{
// 테스트용 임시 디렉토리 관리자
class TempDir
{
public:
    explicit TempDir(StringView name)
    {
        char* pref = SDL_GetPrefPath("SimpleEngine", "Tests");
        root_path = Path(pref) / Path("DDCTest") / Path(name);
        SDL_free(pref);
        fs::CreateDirectories(root_path);
    }

    ~TempDir()
    {
        fs::RemoveAll(root_path);
    }

    [[nodiscard]] const Path& GetPath() const { return root_path; }

private:
    Path root_path;
};

// 테스트용 ContentHash 생성 (라벨 바이트를 그대로 해시에 채워넣음)
ContentHash MakeTestHash(StringView label)
{
    u8 raw[ContentHash::DIGEST_SIZE] = {};
    std::memcpy(raw, label.Data(), std::min(label.ByteLen(), usize{ ContentHash::DIGEST_SIZE }));
    return ContentHash::FromRaw(raw);
}

// 테스트용 페이로드 생성
Array<u8> MakePayload(const std::initializer_list<u8>& data)
{
    Array<u8> result;
    for (u8 byte : data)
    {
        result.Push(byte);
    }
    return result;
}

Array<u8> MakePayload(usize size, u8 fill = 0xAB)
{
    Array<u8> result(size);
    std::memset(result.Data(), fill, size);
    return result;
}
} // namespace


// =============================================================================
// Store / Load 기본 동작
// =============================================================================

class DDCTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "BasicTest" };
    DerivedDataCache ddc{ temp_dir.GetPath() / Path{ "DDC" } };
};

TEST_F(DDCTest, StoreAndLoad)
{
    const Guid guid = Guid::NewGuid();
    const ContentHash hash = MakeTestHash("abcdef1234567890");
    const u32 version = 1;
    const Array<u8> payload = MakePayload({ 0x01, 0x02, 0x03, 0x04 });

    // Store
    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = hash,
        .cache_version = version,
        .payload = payload
    }));

    // Load
    const auto result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    EXPECT_EQ(result->source_hash, hash);
    EXPECT_EQ(result->cache_version, version);
    ASSERT_EQ(result->payload.Len(), payload.Len());
    EXPECT_EQ(std::memcmp(result->payload.Data(), payload.Data(), payload.Len()), 0);
}

TEST_F(DDCTest, LoadNonExistent)
{
    const Guid guid = Guid::NewGuid();
    const auto result = ddc.Load(guid);
    EXPECT_FALSE(result.HasValue());
}

TEST_F(DDCTest, Contains)
{
    const Guid guid = Guid::NewGuid();
    EXPECT_FALSE(ddc.Contains(guid));

    const Array<u8> payload = MakePayload({ 0xFF });
    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("test"),
        .cache_version = 1,
        .payload = payload
    }));

    EXPECT_TRUE(ddc.Contains(guid));
}


// =============================================================================
// 유효성 검증
// =============================================================================

TEST_F(DDCTest, IsValid_MatchingHashAndVersion)
{
    const Guid guid = Guid::NewGuid();
    const ContentHash hash = MakeTestHash("matching_hash");
    const u32 version = 3;
    const Array<u8> payload = MakePayload(16);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = hash,
        .cache_version = version,
        .payload = payload
    }));

    EXPECT_TRUE(ddc.IsValid(guid, hash, version));
}

TEST_F(DDCTest, IsValid_MismatchHash)
{
    const Guid guid = Guid::NewGuid();
    const Array<u8> payload = MakePayload(16);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("original"),
        .cache_version = 1,
        .payload = payload
    }));

    EXPECT_FALSE(ddc.IsValid(guid, MakeTestHash("different"), 1));
}

TEST_F(DDCTest, IsValid_MismatchVersion)
{
    const Guid guid = Guid::NewGuid();
    const Array<u8> payload = MakePayload(16);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("same"),
        .cache_version = 1,
        .payload = payload
    }));

    EXPECT_FALSE(ddc.IsValid(guid, MakeTestHash("same"), 2));
}

TEST_F(DDCTest, IsValid_NonExistent)
{
    const Guid guid = Guid::NewGuid();
    EXPECT_FALSE(ddc.IsValid(guid, MakeTestHash("any"), 1));
}


// =============================================================================
// 덮어쓰기
// =============================================================================

TEST_F(DDCTest, OverwriteExistingCache)
{
    const Guid guid = Guid::NewGuid();
    const Array<u8> payload1 = MakePayload({ 0x01, 0x02 });
    const Array<u8> payload2 = MakePayload({ 0xAA, 0xBB, 0xCC });

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("v1"),
        .cache_version = 1,
        .payload = payload1
    }));
    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("v2"),
        .cache_version = 2,
        .payload = payload2
    }));

    const auto result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    EXPECT_EQ(result->source_hash, MakeTestHash("v2"));
    EXPECT_EQ(result->cache_version, 2u);
    ASSERT_EQ(result->payload.Len(), payload2.Len());
    EXPECT_EQ(std::memcmp(result->payload.Data(), payload2.Data(), payload2.Len()), 0);
}


// =============================================================================
// 삭제
// =============================================================================

TEST_F(DDCTest, Remove)
{
    const Guid guid = Guid::NewGuid();
    const Array<u8> payload = MakePayload({ 0x01 });

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("test"),
        .cache_version = 1,
        .payload = payload
    }));
    EXPECT_TRUE(ddc.Contains(guid));

    EXPECT_TRUE(ddc.Remove(guid));
    EXPECT_FALSE(ddc.Contains(guid));
}

TEST_F(DDCTest, RemoveNonExistent)
{
    const Guid guid = Guid::NewGuid();
    // 존재하지 않는 파일 삭제는 성공으로 처리
    EXPECT_TRUE(ddc.Remove(guid));
}

TEST_F(DDCTest, Clear)
{
    const Guid guid1 = Guid::NewGuid();
    const Guid guid2 = Guid::NewGuid();
    const Array<u8> payload = MakePayload(8);

    ASSERT_TRUE(ddc.Store(guid1, {
        .source_hash = MakeTestHash("a"),
        .cache_version = 1,
        .payload = payload
    }));
    ASSERT_TRUE(ddc.Store(guid2, {
        .source_hash = MakeTestHash("b"),
        .cache_version = 1,
        .payload = payload
    }));

    EXPECT_TRUE(ddc.Contains(guid1));
    EXPECT_TRUE(ddc.Contains(guid2));

    ddc.Clear();

    EXPECT_FALSE(ddc.Contains(guid1));
    EXPECT_FALSE(ddc.Contains(guid2));
}


// =============================================================================
// 빈 페이로드
// =============================================================================

TEST_F(DDCTest, EmptyPayload)
{
    const Guid guid = Guid::NewGuid();
    const Array<u8> empty_payload;

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("empty"),
        .cache_version = 1,
        .payload = empty_payload
    }));

    const auto result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    EXPECT_EQ(result->source_hash, MakeTestHash("empty"));
    EXPECT_EQ(result->payload.Len(), 0u);
}


// =============================================================================
// 큰 페이로드
// =============================================================================

TEST_F(DDCTest, LargePayload)
{
    const Guid guid = Guid::NewGuid();
    const usize large_size = 1024 * 1024; // 1MB
    const Array<u8> payload = MakePayload(large_size, 0xCD);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = MakeTestHash("large"),
        .cache_version = 1,
        .payload = payload
    }));

    const auto result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    ASSERT_EQ(result->payload.Len(), large_size);
    EXPECT_EQ(std::memcmp(result->payload.Data(), payload.Data(), large_size), 0);
}


// =============================================================================
// 여러 GUID 독립성
// =============================================================================

TEST_F(DDCTest, MultipleGuidsIndependent)
{
    const Guid guid1 = Guid::NewGuid();
    const Guid guid2 = Guid::NewGuid();

    const Array<u8> payload1 = MakePayload({ 0x11, 0x22 });
    const Array<u8> payload2 = MakePayload({ 0xAA, 0xBB, 0xCC });

    ASSERT_TRUE(ddc.Store(guid1, {
        .source_hash = MakeTestHash("hash1"),
        .cache_version = 1,
        .payload = payload1
    }));
    ASSERT_TRUE(ddc.Store(guid2, {
        .source_hash = MakeTestHash("hash2"),
        .cache_version = 2,
        .payload = payload2
    }));

    const auto r1 = ddc.Load(guid1);
    const auto r2 = ddc.Load(guid2);

    ASSERT_TRUE(r1.HasValue());
    ASSERT_TRUE(r2.HasValue());

    EXPECT_EQ(r1->source_hash, MakeTestHash("hash1"));
    EXPECT_EQ(r1->cache_version, 1u);
    EXPECT_EQ(r1->payload.Len(), 2u);

    EXPECT_EQ(r2->source_hash, MakeTestHash("hash2"));
    EXPECT_EQ(r2->cache_version, 2u);
    EXPECT_EQ(r2->payload.Len(), 3u);
}


// =============================================================================
// DDC 루트 디렉토리 자동 생성
// =============================================================================

TEST(DDCInitTest, CreateRootDirectoryOnConstruction)
{
    TempDir temp{ "InitTest" };
    const Path ddc_root = temp.GetPath() / Path{ "NewDDC" } / Path{ "SubDir" };

    EXPECT_FALSE(ddc_root.Exists());

    // DDC 생성 시 루트 디렉토리가 자동으로 생성되어야 함
    DerivedDataCache ddc{ ddc_root };

    EXPECT_TRUE(ddc_root.Exists());
    EXPECT_TRUE(ddc_root.IsDirectory());
}


// =============================================================================
// 에셋 payload의 전체 경로 (SerializeAssetPayload -> Store -> Load -> DeserializeAssetPayload)
// =============================================================================

namespace
{
/** original을 payload로 써서 DDC에 저장한 뒤, 다시 읽어 역직렬화한 값이 원본과 같은지 확인합니다. */
template <typename T>
void ExpectCacheRoundTrip(DerivedDataCache& ddc, const T& original)
{
    SCOPED_TRACE(std::string_view{ TypeId_v1::Of<T>().GetName() });

    const Guid guid = Guid::NewGuid();
    const ContentHash hash = MakeTestHash("source");
    Array<u8> payload = AssetSubsystem::SerializeAssetPayload(original);
    ASSERT_FALSE(payload.IsEmpty());
    ASSERT_TRUE(ddc.Store(guid, { .source_hash = hash, .cache_version = 1, .payload = std::move(payload) }));

    ASSERT_TRUE(ddc.IsValid(guid, hash, 1));
    const auto entry = ddc.Load(guid);
    ASSERT_TRUE(entry.HasValue());

    const AssetPayload loaded = AssetSubsystem::DeserializeAssetPayload(TypeId_v1::Of<T>(), entry->payload);
    ASSERT_TRUE(loaded.IsValid());
    EXPECT_EQ(test_assets::WriteToml(*static_cast<const T*>(loaded.ptr)), test_assets::WriteToml(original));
    loaded.destructor(loaded.ptr);
}

/** StaticMesh payload를 guid로 저장하고, 캐시 파일의 바이트를 돌려줍니다. */
[[nodiscard]] Array<u8> StoreStaticMesh(DerivedDataCache& ddc, const Guid& guid, const ContentHash& hash)
{
    EXPECT_TRUE(ddc.Store(guid, {
        .source_hash = hash,
        .cache_version = 1,
        .payload = AssetSubsystem::SerializeAssetPayload(test_assets::MakeStaticMesh()),
    }));

    const auto file = fs::ReadBytes(ddc.BuildCachePath(guid));
    EXPECT_TRUE(file.HasValue());
    return file.HasValue() ? *file : Array<u8>{};
}

/** guid의 캐시 파일을 IsValid 검사 없이 읽어 StaticMesh로 역직렬화할 수 있는지 돌려줍니다. */
[[nodiscard]] bool CanDeserializeStaticMesh(const DerivedDataCache& ddc, const Guid& guid)
{
    const auto entry = ddc.Load(guid);
    if (!entry.HasValue())
    {
        return false;
    }

    const AssetPayload loaded = AssetSubsystem::DeserializeAssetPayload(TypeId_v1::Of<StaticMesh>(), entry->payload);
    if (!loaded.IsValid())
    {
        return false;
    }
    loaded.destructor(loaded.ptr);
    return true;
}
} // namespace

TEST_F(DDCTest, EveryAssetTypeRoundTripsThroughCache)
{
    ExpectCacheRoundTrip(ddc, test_assets::MakeStaticMesh());
    ExpectCacheRoundTrip(ddc, test_assets::MakeSkeletalMesh());
    ExpectCacheRoundTrip(ddc, test_assets::MakeTexture2D());
    ExpectCacheRoundTrip(ddc, test_assets::MakeMaterial());
    ExpectCacheRoundTrip(ddc, test_assets::MakeMaterialInstance());
}

TEST_F(DDCTest, LegacyFormatFileIsMiss)
{
    const Guid guid = Guid::NewGuid();
    const ContentHash hash = MakeTestHash("source");
    std::ignore = StoreStaticMesh(ddc, guid, hash);

    // 예전 형식처럼 "SEDC" magic과 format_version 1, cache_version 1로 시작하는 파일로 덮어씀
    Array<u8> legacy_file = { 'S', 'E', 'D', 'C', 1, 0, 0, 0, 1, 0, 0, 0 };
    legacy_file.Resize(legacy_file.Len() + 64);
    ASSERT_TRUE(fs::Write(ddc.BuildCachePath(guid), legacy_file));

    EXPECT_FALSE(ddc.IsValid(guid, hash, 1));
    EXPECT_FALSE(CanDeserializeStaticMesh(ddc, guid));
}

TEST_F(DDCTest, CorruptedPayloadIsMiss)
{
    const Guid guid = Guid::NewGuid();
    const ContentHash hash = MakeTestHash("source");
    Array<u8> file = StoreStaticMesh(ddc, guid, hash);
    ASSERT_FALSE(file.IsEmpty());

    // 마지막 바이트만 바꾸므로 앞부분 검사는 통과하고 Packed 헤더의 체크섬에서 거절됨
    file[file.Len() - 1] = static_cast<u8>(~file[file.Len() - 1]);
    ASSERT_TRUE(fs::Write(ddc.BuildCachePath(guid), file));

    EXPECT_TRUE(ddc.IsValid(guid, hash, 1));
    EXPECT_FALSE(CanDeserializeStaticMesh(ddc, guid));
}

TEST_F(DDCTest, TruncatedFileIsMiss)
{
    const Guid guid = Guid::NewGuid();
    const ContentHash hash = MakeTestHash("source");
    const Array<u8> file = StoreStaticMesh(ddc, guid, hash);
    ASSERT_GT(file.Len(), 100u);

    // 앞부분보다 짧으면 IsValid와 Load가 모두 실패
    ASSERT_TRUE(fs::Write(ddc.BuildCachePath(guid), ArrayView<const u8>(file.Data(), 10)));
    EXPECT_FALSE(ddc.IsValid(guid, hash, 1));
    EXPECT_FALSE(ddc.Load(guid).HasValue());

    // payload 중간에서 잘리면 Packed 헤더의 크기 검사에서 거절됨
    ASSERT_TRUE(fs::Write(ddc.BuildCachePath(guid), ArrayView<const u8>(file.Data(), file.Len() - 5)));
    EXPECT_TRUE(ddc.IsValid(guid, hash, 1));
    EXPECT_FALSE(CanDeserializeStaticMesh(ddc, guid));
}

TEST(AssetPayloadTest, PayloadOfAnotherAssetTypeIsRejected)
{
    // 루트 타입이 헤더와 다르면 필드를 읽기 전에 거절됨
    const Array<u8> payload = AssetSubsystem::SerializeAssetPayload(test_assets::MakeStaticMesh());
    EXPECT_FALSE(AssetSubsystem::DeserializeAssetPayload(TypeId_v1::Of<SkeletalMesh>(), payload).IsValid());
}

TEST(AssetPayloadTest, WorkerThreadsDeserializePayloadsConcurrently)
{
    // 비동기 로드는 워커 스레드에서 payload를 역직렬화하므로, 여러 스레드가 여러 에셋 타입을 동시에 읽어도 모두 성공해야 함
    struct TypedPayload
    {
        TypeId_v1 type;
        Array<u8> bytes;
    };
    const TypedPayload payloads[] = {
        { TypeId_v1::Of<StaticMesh>(), AssetSubsystem::SerializeAssetPayload(test_assets::MakeStaticMesh()) },
        { TypeId_v1::Of<SkeletalMesh>(), AssetSubsystem::SerializeAssetPayload(test_assets::MakeSkeletalMesh()) },
        { TypeId_v1::Of<Texture2D>(), AssetSubsystem::SerializeAssetPayload(test_assets::MakeTexture2D()) },
        { TypeId_v1::Of<Material>(), AssetSubsystem::SerializeAssetPayload(test_assets::MakeMaterial()) },
        { TypeId_v1::Of<MaterialInstance>(), AssetSubsystem::SerializeAssetPayload(test_assets::MakeMaterialInstance()) },
    };

    constexpr usize THREAD_COUNT = 8;
    constexpr usize ROUND_COUNT = 20;
    std::atomic<usize> failures{ 0 };
    {
        Array<std::jthread> threads;
        threads.Reserve(THREAD_COUNT);
        for (usize thread_index = 0; thread_index < THREAD_COUNT; ++thread_index)
        {
            threads.Emplace([&payloads, &failures]
            {
                for (usize round = 0; round < ROUND_COUNT; ++round)
                {
                    for (const TypedPayload& payload : payloads)
                    {
                        const AssetPayload loaded = AssetSubsystem::DeserializeAssetPayload(payload.type, payload.bytes);
                        if (!loaded.IsValid())
                        {
                            failures.fetch_add(1, std::memory_order_relaxed);
                            continue;
                        }
                        loaded.destructor(loaded.ptr);
                    }
                }
            });
        }
    }

    EXPECT_EQ(failures.load(), 0u);
}
