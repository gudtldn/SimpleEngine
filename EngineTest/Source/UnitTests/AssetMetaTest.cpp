#include "gtest/gtest.h"

#include "SimpleEditor/Asset/AssetMeta.h"
#include "SimpleEditor/Asset/ImportProfile.h"
#include "SimpleEditor/Asset/ImportSettings/ImportSettingsBase.h"
#include "SimpleEditor/Asset/ImportSettings/MeshImportSettings.h"
#include "SimpleEditor/Asset/MetaFileContent.h"
#include "SimpleEditor/Asset/Pipeline/ProcessorEntry.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/MeshTypes.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Utility/Common.h"
#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"

#include "SDL3/SDL_filesystem.h"

#include <ranges>
#include <string>
#include <string_view>

using namespace se;
using namespace se::editor;

// .meta 파일은 새 직렬화로 읽고 쓰지만 기존 파일과 같은 텍스트여야 하므로, 실제 .meta 파일의 내용을 골든으로 확인
// .meta의 구조체는 에디터 UI와 임포트 파이프라인이 쓰는 레거시 리플렉션에도 등록되므로, 한쪽에만 필드를 추가하는 실수를 잡기 위해 두 등록을 비교
namespace
{
/** EngineCore/Assets/Cube.obj.meta의 내용 그대로입니다. */
constexpr std::string_view CUBE_META_TEXT =
    "processor_stack = []\n"
    "\n"
    "[import_settings.'se::editor::MeshImportSettings']\n"
    "apply_transform = true\n"
    "combine_meshes = true\n"
    "global_scale = 1.0\n"
    "\n"
    "[metadata]\n"
    "cache_version = 1\n"
    "guid = '0dd9f95f-f684-4b35-8f42-e2ce2d9f52ab'\n"
    "settings_hash = '82a967f4e418eb518b3e599736e29c936cf3452045c20b375a5d854590bc417a'\n"
    "source_hash = '44cef8efa27cc51242067df382cc419a7b50e43e866ef5b467f7efa46eaddeeb'\n"
    "source_mtime = 1775110252411093200\n"
    "source_size = 945\n"
    "\n"
    "    [[metadata.sub_assets]]\n"
    "    dependencies = []\n"
    "    guid = 'df89d951-dc57-4bc7-90d8-df460daea1b8'\n"
    "    name = 'Cube'\n"
    "    type = 'se::StaticMesh'\n"
    "\n"
    "    [[metadata.sub_assets]]\n"
    "    dependencies = []\n"
    "    guid = '86cf11d2-ee50-46c9-ac18-76ca5172c7c1'\n"
    "    name = 'Material_DefaultMaterial'\n"
    "    type = 'se::MaterialInstance'";

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

/** TOML 텍스트를 테이블로 파싱합니다. 문법 오류면 테스트를 실패시키고 빈 테이블을 돌려줍니다. */
[[nodiscard]] toml::table ParseToml(std::string_view text)
{
    toml::parse_result result = toml::parse(text);
    if (!result)
    {
        ADD_FAILURE() << "TOML parse error: " << result.error().description();
        return {};
    }
    return std::move(result).table();
}

/** content를 TOML 테이블로 씁니다. */
[[nodiscard]] toml::table WriteToml(const MetaFileContent& content)
{
    toml::table table;
    TomlWriter writer(table);
    const auto written = serde::Serialize(writer, content);
    EXPECT_TRUE(written.HasValue()) << (written.HasError() ? written.Error().message.CStr() : "");
    return table;
}

/** profile에 MeshImportSettings 하나만 있고, 그 값이 expected와 같은지 확인합니다. */
void ExpectOnlyMeshSettings(const ImportProfile& profile, const MeshImportSettings& expected)
{
    EXPECT_EQ(profile.GetSettingsMap().Len(), 1);

    const auto mesh = profile.Get<MeshImportSettings>();
    ASSERT_TRUE(mesh.HasValue());
    EXPECT_EQ(mesh->combine_meshes, expected.combine_meshes);
    EXPECT_EQ(mesh->apply_transform, expected.apply_transform);
    EXPECT_EQ(mesh->global_scale, expected.global_scale);
}
} // namespace


TEST(AssetMetaTest, BothRegistrationsListSameFields)
{
    const DualTypeIds meta_types[] = {
        IdsOf<ImportSettingsBase>(),
        IdsOf<MeshImportSettings>(),
        IdsOf<ProcessorEntry>(),
        IdsOf<MetaFileContent>(),
    };

    for (const DualTypeIds& type : meta_types)
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

TEST(AssetMetaTest, CubeMetaLoadsWithoutWarnings)
{
    const toml::table table = ParseToml(CUBE_META_TEXT);

    SkippedImportSettings skipped;
    SerializeContext context;
    context.Add(skipped);

    TomlReader reader(table);
    reader.SetContext(&context);
    MetaFileContent content;
    ASSERT_TRUE(serde::Deserialize(reader, content).HasValue());

    // 기존 파일의 키가 모두 타입에 있어 경고가 0건이고, 건너뛴 설정도 없음
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
    EXPECT_TRUE(skipped.type_names.IsEmpty());

    const AssetMetadata expected_metadata{
        .guid = Guid::FromString("0dd9f95f-f684-4b35-8f42-e2ce2d9f52ab"),
        .source_hash = ContentHash::FromHex("44cef8efa27cc51242067df382cc419a7b50e43e866ef5b467f7efa46eaddeeb"),
        .source_mtime = 1775110252411093200,
        .source_size = 945,
        .cache_version = 1,
        .settings_hash = ContentHash::FromHex("82a967f4e418eb518b3e599736e29c936cf3452045c20b375a5d854590bc417a"),
        .sub_assets = {
            SubAssetMeta{
                .name = "Cube",
                .guid = Guid::FromString("df89d951-dc57-4bc7-90d8-df460daea1b8"),
                .type = TypeId_v1::Of<StaticMesh>(),
            },
            SubAssetMeta{
                .name = "Material_DefaultMaterial",
                .guid = Guid::FromString("86cf11d2-ee50-46c9-ac18-76ca5172c7c1"),
                .type = TypeId_v1::Of<MaterialInstance>(),
            },
        },
    };
    EXPECT_EQ(content.metadata, expected_metadata);
    ExpectOnlyMeshSettings(content.import_settings, MeshImportSettings{});
    EXPECT_TRUE(content.processor_stack.IsEmpty());
}

TEST(AssetMetaTest, CubeMetaSavesSameText)
{
    char* const pref_path = SDL_GetPrefPath("SimpleEngine", "Tests");
    const Path dir = Path(pref_path) / Path("AssetMetaTest");
    SDL_free(pref_path);
    fs::CreateDirectories(dir);
    SE_SCOPE_DEFER {
        fs::RemoveAll(dir);
    };

    // Load와 Save는 소스 파일 옆의 .meta만 다루므로 소스 파일은 만들지 않음
    const Path source_path = dir / Path("Cube.obj");
    ASSERT_TRUE(fs::WriteString(asset_meta::MetaPathOf(source_path), CUBE_META_TEXT));

    const auto content = asset_meta::Load(source_path);
    ASSERT_TRUE(content.HasValue());
    ASSERT_TRUE(asset_meta::Save(source_path, *content));

    const auto saved_text = fs::ReadToString(asset_meta::MetaPathOf(source_path));
    ASSERT_TRUE(saved_text.HasValue());
    EXPECT_EQ(saved_text.Value(), StringView{ CUBE_META_TEXT });
}

TEST(AssetMetaTest, ImportSettingsRoundTrip)
{
    MeshImportSettings mesh;
    mesh.combine_meshes = false;
    mesh.apply_transform = false;
    mesh.global_scale = 2.5f;

    MetaFileContent original;
    original.import_settings.Set(mesh);

    // TOML에서는 설정 타입 이름이 테이블 키
    const toml::table table = WriteToml(original);
    EXPECT_EQ(table["import_settings"]["se::editor::MeshImportSettings"]["global_scale"].value_exact<f64>(), 2.5);

    TomlReader toml_reader(table);
    MetaFileContent from_toml;
    ASSERT_TRUE(serde::Deserialize(toml_reader, from_toml).HasValue());
    ExpectOnlyMeshSettings(from_toml.import_settings, mesh);

    Array<u8> buffer;
    PackedWriter packed_writer(buffer);
    ASSERT_TRUE(serde::Serialize(packed_writer, original.import_settings).HasValue());

    PackedReader packed_reader(buffer);
    ImportProfile from_packed;
    ASSERT_TRUE(serde::Deserialize(packed_reader, from_packed).HasValue());
    ExpectOnlyMeshSettings(from_packed, mesh);
}

TEST(AssetMetaTest, UnknownSettingsTypeIsSkipped)
{
    // 레거시에 등록됐지만 ImportSettingsBase 파생이 아닌 타입과, 코드에서 사라진 설정 타입
    const toml::table table = ParseToml(
        "[import_settings.'se::StaticMesh']\n"
        "vertices = []\n"
        "\n"
        "[import_settings.'se::editor::MeshImportSettings']\n"
        "global_scale = 2.0\n"
        "\n"
        "[import_settings.'se::editor::RemovedImportSettings']\n"
        "strength = 3\n"
    );

    SkippedImportSettings skipped;
    SerializeContext context;
    context.Add(skipped);

    TomlReader reader(table);
    reader.SetContext(&context);
    MetaFileContent content;
    ASSERT_TRUE(serde::Deserialize(reader, content).HasValue());

    // 아는 설정만 읽고, 나머지는 value를 읽지 않고 건너뛰어 이름만 키 순서로 기록됨
    MeshImportSettings expected_mesh;
    expected_mesh.global_scale = 2.0f;
    ExpectOnlyMeshSettings(content.import_settings, expected_mesh);

    ASSERT_EQ(skipped.type_names.Len(), 2);
    EXPECT_EQ(skipped.type_names[0], "se::StaticMesh");
    EXPECT_EQ(skipped.type_names[1], "se::editor::RemovedImportSettings");
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
}

TEST(AssetMetaTest, UnknownSettingsTypeFailsInPacked)
{
    Array<u8> buffer;
    PackedWriter writer(buffer);
    writer.BeginMap(1);
    writer.BeginMapEntry();
    writer.Str("se::editor::RemovedImportSettings");
    writer.Int(3, EIntWidth::Bits32, true);
    writer.EndMapEntry();
    writer.EndMap();

    // 태그 없는 바이너리는 모르는 value의 크기를 몰라 건너뛸 수 없음
    PackedReader reader(buffer);
    ImportProfile profile;
    EXPECT_TRUE(serde::Deserialize(reader, profile).HasError());
}

TEST(AssetMetaTest, ProcessorStackRoundTrip)
{
    MetaFileContent original;
    original.processor_stack.Push(ProcessorEntry{ .processor_type = TypeId_v1::Of<StaticMesh>(), .enabled = true });
    original.processor_stack.Push(ProcessorEntry{ .processor_type = TypeId_v1::Of<Texture2D>(), .enabled = false });

    // 타입은 레거시에 등록된 이름으로 씀
    const toml::table table = WriteToml(original);
    EXPECT_EQ(table["processor_stack"][1]["processor_type"].value_exact<std::string>(), "se::Texture2D");

    TomlReader reader(table);
    MetaFileContent result;
    ASSERT_TRUE(serde::Deserialize(reader, result).HasValue());

    ASSERT_EQ(result.processor_stack.Len(), 2);
    EXPECT_EQ(result.processor_stack[0].processor_type, TypeId_v1::Of<StaticMesh>());
    EXPECT_TRUE(result.processor_stack[0].enabled);
    EXPECT_EQ(result.processor_stack[1].processor_type, TypeId_v1::Of<Texture2D>());
    EXPECT_FALSE(result.processor_stack[1].enabled);
}

TEST(AssetMetaTest, SettingsHashFollowsSettingsValues)
{
    MeshImportSettings mesh;
    ImportProfile profile;
    profile.Set(mesh);
    ImportProfile same_profile;
    same_profile.Set(mesh);

    mesh.global_scale = 2.0f;
    ImportProfile changed_profile;
    changed_profile.Set(mesh);

    // 같은 설정이면 같은 해시이고, 값이 바뀌거나 설정이 빠지면 다른 해시
    EXPECT_EQ(profile.ComputeHash(), same_profile.ComputeHash());
    EXPECT_NE(profile.ComputeHash(), changed_profile.ComputeHash());
    EXPECT_NE(profile.ComputeHash(), ImportProfile{}.ComputeHash());
}
