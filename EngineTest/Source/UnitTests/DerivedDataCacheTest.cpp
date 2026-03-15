#include "gtest/gtest.h"

#include "SimpleEngine/Asset/DerivedDataCache.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Types/Guid.h"

#include "SDL3/SDL_filesystem.h"

using namespace se;
using namespace se::asset;


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
        FileSystem::CreateDirectories(root_path);
    }

    ~TempDir()
    {
        FileSystem::RemoveAll(root_path);
    }

    [[nodiscard]] const Path& GetPath() const { return root_path; }

private:
    Path root_path;
};

// 테스트용 페이로드 생성
Array<uint8> MakePayload(const std::initializer_list<uint8>& data)
{
    Array<uint8> result;
    for (uint8 byte : data)
    {
        result.Push(byte);
    }
    return result;
}

Array<uint8> MakePayload(usize size, uint8 fill = 0xAB)
{
    Array<uint8> result(size);
    std::memset(result.Data(), fill, size);
    return result;
}
}  // namespace


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
    const String hash = "sha256:abcdef1234567890";
    constexpr uint32 version = 1;
    const Array<uint8> payload = MakePayload({ 0x01, 0x02, 0x03, 0x04 });

    // Store
    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = hash,
        .cache_version = version,
        .payload = payload
    }));

    // Load
    const Optional result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    EXPECT_EQ(result->source_hash, hash);
    EXPECT_EQ(result->cache_version, version);
    ASSERT_EQ(result->payload.Len(), payload.Len());
    EXPECT_EQ(std::memcmp(result->payload.Data(), payload.Data(), payload.Len()), 0);
}

TEST_F(DDCTest, LoadNonExistent)
{
    const Guid guid = Guid::NewGuid();
    const Optional result = ddc.Load(guid);
    EXPECT_FALSE(result.HasValue());
}

TEST_F(DDCTest, Contains)
{
    const Guid guid = Guid::NewGuid();
    EXPECT_FALSE(ddc.Contains(guid));

    const Array<uint8> payload = MakePayload({ 0xFF });
    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:test",
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
    const String hash = "sha256:matching_hash";
    constexpr uint32 version = 3;
    const Array<uint8> payload = MakePayload(16);

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
    const Array<uint8> payload = MakePayload(16);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:original",
        .cache_version = 1,
        .payload = payload
    }));

    EXPECT_FALSE(ddc.IsValid(guid, "sha256:different", 1));
}

TEST_F(DDCTest, IsValid_MismatchVersion)
{
    const Guid guid = Guid::NewGuid();
    const Array<uint8> payload = MakePayload(16);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:same",
        .cache_version = 1,
        .payload = payload
    }));

    EXPECT_FALSE(ddc.IsValid(guid, "sha256:same", 2));
}

TEST_F(DDCTest, IsValid_NonExistent)
{
    const Guid guid = Guid::NewGuid();
    EXPECT_FALSE(ddc.IsValid(guid, "sha256:any", 1));
}


// =============================================================================
// 덮어쓰기
// =============================================================================

TEST_F(DDCTest, OverwriteExistingCache)
{
    const Guid guid = Guid::NewGuid();
    const Array<uint8> payload1 = MakePayload({ 0x01, 0x02 });
    const Array<uint8> payload2 = MakePayload({ 0xAA, 0xBB, 0xCC });

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:v1",
        .cache_version = 1,
        .payload = payload1
    }));
    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:v2",
        .cache_version = 2,
        .payload = payload2
    }));

    const Optional result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    EXPECT_EQ(result->source_hash, "sha256:v2");
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
    const Array<uint8> payload = MakePayload({ 0x01 });

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:test",
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
    const Array<uint8> payload = MakePayload(8);

    ASSERT_TRUE(ddc.Store(guid1, {
        .source_hash = "sha256:a",
        .cache_version = 1,
        .payload = payload
    }));
    ASSERT_TRUE(ddc.Store(guid2, {
        .source_hash = "sha256:b",
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
    const Array<uint8> empty_payload;

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:empty",
        .cache_version = 1,
        .payload = empty_payload
    }));

    const Optional result = ddc.Load(guid);
    ASSERT_TRUE(result.HasValue());

    EXPECT_EQ(result->source_hash, "sha256:empty");
    EXPECT_EQ(result->payload.Len(), 0u);
}


// =============================================================================
// 큰 페이로드
// =============================================================================

TEST_F(DDCTest, LargePayload)
{
    const Guid guid = Guid::NewGuid();
    constexpr usize large_size = 1024 * 1024; // 1MB
    const Array<uint8> payload = MakePayload(large_size, 0xCD);

    ASSERT_TRUE(ddc.Store(guid, {
        .source_hash = "sha256:large",
        .cache_version = 1,
        .payload = payload
    }));

    const Optional result = ddc.Load(guid);
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

    const Array<uint8> payload1 = MakePayload({ 0x11, 0x22 });
    const Array<uint8> payload2 = MakePayload({ 0xAA, 0xBB, 0xCC });

    ASSERT_TRUE(ddc.Store(guid1, {
        .source_hash = "sha256:hash1",
        .cache_version = 1,
        .payload = payload1
    }));
    ASSERT_TRUE(ddc.Store(guid2, {
        .source_hash = "sha256:hash2",
        .cache_version = 2,
        .payload = payload2
    }));

    const Optional r1 = ddc.Load(guid1);
    const Optional r2 = ddc.Load(guid2);

    ASSERT_TRUE(r1.HasValue());
    ASSERT_TRUE(r2.HasValue());

    EXPECT_EQ(r1->source_hash, "sha256:hash1");
    EXPECT_EQ(r1->cache_version, 1u);
    EXPECT_EQ(r1->payload.Len(), 2u);

    EXPECT_EQ(r2->source_hash, "sha256:hash2");
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
