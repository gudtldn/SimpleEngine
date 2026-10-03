#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/AssetPath.h"
#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include "SDL3/SDL_filesystem.h"

#include <cstring>
#include <ranges>
#include <string>
#include <string_view>

using namespace se;

// 레지스트리 스냅샷은 AssetRecord를 새 직렬화의 Binary 파일로 저장하므로, 스냅샷 왕복과 예전 형식, 손상된 파일의 거절을 검증
// 메타데이터 타입은 .meta와 에디터가 쓰는 레거시 리플렉션에도 등록되어 있어, 한쪽에만 필드를 추가하는 실수를 잡기 위해 두 등록을 비교
namespace
{
/** 타입 하나를 두 레지스트리에서 찾을 TypeId */
struct DualTypeIds
{
    TypeId id;
    TypeId_v1 legacy_id;
};

/** T의 두 TypeId를 만듭니다. 둘 다 타입 이름의 해시라 등록 템플릿을 인스턴스화하지 않습니다. */
template <typename T>
[[nodiscard]] DualTypeIds IdsOf()
{
    return { .id = TypeId::Of<T>(), .legacy_id = TypeId_v1::Of<T>() };
}

/** 이름들을 ", "로 이어 붙입니다. 실패 메시지에 두 목록이 그대로 보이도록 문자열로 비교합니다. */
template <std::ranges::input_range Names>
[[nodiscard]] std::string JoinNames(Names&& names)
{
    std::string joined;
    for (const StringView name : names)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += std::string_view{ name };
    }
    return joined;
}

/** SDL 사용자 경로 아래의 테스트용 파일 경로. 소멸할 때 디렉터리째 지웁니다. */
class TempFile
{
public:
    explicit TempFile(StringView name)
    {
        char* const pref = SDL_GetPrefPath("SimpleEngine", "Tests");
        directory = Path(pref) / Path("AssetRegistrySnapshotTest");
        SDL_free(pref);
        fs::CreateDirectories(directory);
        path = directory / Path(name);
    }

    ~TempFile()
    {
        fs::RemoveAll(directory);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;

    [[nodiscard]] const Path& GetPath() const { return path; }

private:
    Path directory;
    Path path;
};

/** 모든 바이트가 fill인 ContentHash를 만듭니다. */
[[nodiscard]] ContentHash MakeHash(const u8 fill)
{
    u8 raw[ContentHash::DIGEST_SIZE] = {};
    std::memset(raw, fill, sizeof(raw));
    return ContentHash::FromRaw(raw);
}

/** sub-asset 하나와 의존성 하나를 가진 메타데이터를 만듭니다. */
[[nodiscard]] AssetMetadata MakeMetadata(const AssetId& id, const TypeId_v1& type)
{
    return {
        .guid = id.GetGuid(),
        .source_hash = MakeHash(0x11),
        .source_mtime = 1'700'000'000,
        .source_size = 4096,
        .cache_version = 1,
        .settings_hash = MakeHash(0x22),
        .sub_assets = {
            SubAssetMeta{
                .name = "Sub",
                .guid = id.GetGuid(),
                .type = type,
                .dependencies = {
                    AssetDependencyEntry{
                        .source_vpath = "Assets://Textures/Wood.png",
                        .asset_guid = Guid::NewGuid(),
                        .type = EAssetDependencyType::Soft,
                    },
                },
            },
        },
    };
}

/** 두 레지스트리에서 id의 레코드가 같은지 확인합니다. */
void ExpectSameRecord(const AssetRegistry& expected, const AssetRegistry& actual, const AssetId& id)
{
    AssetRecord expected_record;
    ASSERT_TRUE(expected.ReadRecord(id, [&expected_record](const AssetRecord& record) { expected_record = record; }));

    const bool found = actual.ReadRecord(id, [&expected_record](const AssetRecord& record)
    {
        EXPECT_EQ(record.type, expected_record.type);
        EXPECT_EQ(record.logical_path, expected_record.logical_path);
        EXPECT_EQ(record.metadata, expected_record.metadata);
    });
    EXPECT_TRUE(found);
}
} // namespace


TEST(AssetRegistrySnapshotTest, BothRegistrationsListSameFields)
{
    const DualTypeIds snapshot_types[] = {
        IdsOf<AssetDependencyEntry>(),
        IdsOf<SubAssetMeta>(),
        IdsOf<AssetMetadata>(),
        IdsOf<AssetRecord>(),
    };

    for (const DualTypeIds& type : snapshot_types)
    {
        const auto legacy_info = TypeRegistry_v1::Get().Find(type.legacy_id);
        ASSERT_TRUE(legacy_info.HasValue());
        SCOPED_TRACE(std::string_view{ legacy_info->name });

        const auto info = TypeRegistry::Get().Find(type.id);
        ASSERT_TRUE(info.HasValue()) << "The type is not registered with SE_REFLECT_BEGIN.";
        const auto struct_info = info->AsStruct();
        ASSERT_TRUE(struct_info.HasValue());

        // 이름과 순서가 모두 같아야 함
        EXPECT_EQ(
            JoinNames(struct_info->fields | std::views::transform(&FieldInfo::name)),
            JoinNames(legacy_info->properties | std::views::transform(&PropertyInfo_v1::name))
        );
    }
}

TEST(AssetRegistrySnapshotTest, SnapshotRoundTripRestoresRecordsAndIndexes)
{
    const TempFile file{ "RoundTrip.bin" };

    AssetRegistry original;
    const AssetId mesh_id{ Guid::NewGuid() };
    const AssetId texture_id{ Guid::NewGuid() };
    original.RegisterAsset(mesh_id, TypeId_v1::Of<StaticMesh>(), AssetPath{ "Assets://Hero.fbx#Mesh_Body" }, MakeMetadata(mesh_id, TypeId_v1::Of<StaticMesh>()));
    original.RegisterAsset(texture_id, TypeId_v1::Of<Texture2D>(), AssetPath{ "Assets://Wood.png" }, MakeMetadata(texture_id, TypeId_v1::Of<Texture2D>()));
    ASSERT_TRUE(original.SaveToFile(file.GetPath()));

    AssetRegistry loaded;
    ASSERT_TRUE(loaded.LoadFromFile(file.GetPath()));
    EXPECT_EQ(loaded.GetAssetCount(), 2u);
    ExpectSameRecord(original, loaded, mesh_id);
    ExpectSameRecord(original, loaded, texture_id);

    // records만 저장하므로 보조 인덱스는 로드할 때 다시 만들어져야 함
    EXPECT_EQ(loaded.GetAssetId(AssetPath{ "Assets://Hero.fbx#Mesh_Body" }), mesh_id);
    EXPECT_TRUE(loaded.IsFileImported(VPath{ "Assets://Wood.png" }));
}

TEST(AssetRegistrySnapshotTest, LegacySnapshotIsRejected)
{
    const TempFile file{ "Legacy.bin" };

    // 예전 스냅샷처럼 "SEAR" magic과 version 2로 시작하는 파일
    Array<u8> legacy_snapshot = { 'S', 'E', 'A', 'R', 2, 0, 0, 0 };
    legacy_snapshot.Resize(64);
    ASSERT_TRUE(fs::Write(file.GetPath(), legacy_snapshot));

    AssetRegistry registry;
    EXPECT_FALSE(registry.LoadFromFile(file.GetPath()));
    EXPECT_EQ(registry.GetAssetCount(), 0u);
}

TEST(AssetRegistrySnapshotTest, CorruptedSnapshotIsRejectedWithoutTouchingRecords)
{
    const TempFile file{ "Corrupted.bin" };

    AssetRegistry original;
    const AssetId id{ Guid::NewGuid() };
    original.RegisterAsset(id, TypeId_v1::Of<StaticMesh>(), AssetPath{ "Assets://Hero.fbx" }, MakeMetadata(id, TypeId_v1::Of<StaticMesh>()));
    ASSERT_TRUE(original.SaveToFile(file.GetPath()));

    // 마지막 바이트를 바꾸면 체크섬에서 거절됨
    const auto bytes = fs::ReadBytes(file.GetPath());
    ASSERT_TRUE(bytes.HasValue());
    Array<u8> corrupted = *bytes;
    corrupted[corrupted.Len() - 1] = static_cast<u8>(~corrupted[corrupted.Len() - 1]);
    ASSERT_TRUE(fs::Write(file.GetPath(), corrupted));

    // 이미 가진 레코드는 그대로 남아야 함
    AssetRegistry registry;
    const AssetId existing_id{ Guid::NewGuid() };
    registry.RegisterAsset(existing_id, TypeId_v1::Of<Texture2D>(), AssetPath{ "Assets://Wood.png" }, MakeMetadata(existing_id, TypeId_v1::Of<Texture2D>()));

    EXPECT_FALSE(registry.LoadFromFile(file.GetPath()));
    EXPECT_EQ(registry.GetAssetCount(), 1u);
    EXPECT_TRUE(registry.GetAssetType(existing_id).HasValue());
}
