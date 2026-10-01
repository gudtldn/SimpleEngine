#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/ECS/Components/Camera3dComponent.h"
#include "SimpleEngine/ECS/Components/ChildrenComponent.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/MeshMaterialComponent.h"
#include "SimpleEngine/ECS/Components/NameComponent.h"
#include "SimpleEngine/ECS/Components/ParentComponent.h"
#include "SimpleEngine/ECS/Components/PersistentIdComponent.h"
#include "SimpleEngine/ECS/Components/StaticMeshComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/WorldFile.h"

#include <string_view>

using namespace se;


namespace se_world_file_test
{
/** 어느 리플렉션에도 등록하지 않은 컴포넌트 */
struct UnregisteredComponent
{
    i32 value = 0;
};
} // namespace se_world_file_test


namespace
{
/** world를 월드 파일 텍스트로 씁니다. 실패하면 테스트를 실패로 표시하고 빈 문자열을 돌려줍니다. */
[[nodiscard]] String SaveToText(World& world)
{
    WorldFileWriter writer(world);
    auto text = writer.Write();
    if (text.HasError())
    {
        ADD_FAILURE() << text.Error().CStr();
        return {};
    }
    return std::move(text).Value();
}

/** reader로 text를 읽습니다. 실패하면 테스트를 실패로 표시합니다. */
void LoadFromText(WorldFileReader& reader, StringView text)
{
    if (const auto result = reader.Read(text); result.HasError())
    {
        ADD_FAILURE() << result.Error().CStr();
    }
}

/** world에서 이름이 name이고 except가 아닌 엔티티를 찾습니다. 없으면 null Entity를 돌려줍니다. */
[[nodiscard]] Entity FindByName(const World& world, StringView name, Entity except = {})
{
    for (const Entity entity : world.GetAliveEntities())
    {
        const auto name_component = world.TryGetComponent<NameComponent>(entity);
        if (entity != except && name_component && name_component->name == name)
        {
            return entity;
        }
    }
    return {};
}

/** 월드 파일 텍스트에 적힌 엔티티 id를 순서대로 모읍니다. */
[[nodiscard]] Array<i64> ReadEntityIds(StringView text)
{
    Array<i64> ids;
    const toml::parse_result parsed = toml::parse(std::string_view{ text });
    if (!parsed)
    {
        ADD_FAILURE() << parsed.error().description();
        return ids;
    }

    for (const toml::node& entity : *parsed.table().get_as<toml::array>("entities"))
    {
        ids.Push(entity.as_table()->get_as<i64>("id")->get());
    }
    return ids;
}
} // namespace


TEST(WorldFileTest, EmptyWorldRoundTrips)
{
    World src;
    const String text = SaveToText(src);

    World dst;
    WorldFileReader reader(dst);
    LoadFromText(reader, text);

    EXPECT_TRUE(dst.GetAliveEntities().IsEmpty());
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
}

TEST(WorldFileTest, EverySavedComponentRoundTrips)
{
    const AssetId mesh_id{ Guid::NewGuid() };
    const AssetId material_id{ Guid::NewGuid() };

    World src;
    const Entity parent = src.SpawnEntity(
        NameComponent{ .name = "parent" },
        TransformComponent{
            .rotation = Quaternion(0.0, 0.0, 0.5, 0.75),
            .position = Vector3(1.0, 2.0, 3.0),
            .scale = Vector3(2.0, 2.0, 2.0),
        },
        StaticMeshComponent{ .mesh_id = mesh_id, .force_lod = 2 },
        MeshMaterialComponent{ .material_overrides = { material_id, AssetId{} } },
        Camera3dComponent{ .fov = 60.0_deg, .near_plane = 0.5, .far_plane = 500.0 }
    );
    const Entity child_a = src.SpawnEntity(NameComponent{ .name = "child_a" }, ParentComponent{ .parent = parent });
    const Entity child_b = src.SpawnEntity(NameComponent{ .name = "child_b" }, ParentComponent{ .parent = parent });
    src.AddComponent(parent, ChildrenComponent{ .children = { child_a, child_b } });

    World dst;
    WorldFileReader reader(dst);
    LoadFromText(reader, SaveToText(src));
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
    ASSERT_EQ(dst.GetAliveEntities().Len(), 3u);

    const Entity loaded_parent = FindByName(dst, "parent");
    const Entity loaded_child_a = FindByName(dst, "child_a");
    const Entity loaded_child_b = FindByName(dst, "child_b");
    ASSERT_TRUE(loaded_parent.IsValid());
    ASSERT_TRUE(loaded_child_a.IsValid());
    ASSERT_TRUE(loaded_child_b.IsValid());

    const auto& transform = dst.GetComponent<TransformComponent>(loaded_parent);
    EXPECT_DOUBLE_EQ(transform.rotation.z, 0.5);
    EXPECT_DOUBLE_EQ(transform.rotation.w, 0.75);
    EXPECT_DOUBLE_EQ(transform.position.x, 1.0);
    EXPECT_DOUBLE_EQ(transform.position.y, 2.0);
    EXPECT_DOUBLE_EQ(transform.position.z, 3.0);
    EXPECT_DOUBLE_EQ(transform.scale.y, 2.0);

    const auto& mesh = dst.GetComponent<StaticMeshComponent>(loaded_parent);
    EXPECT_EQ(mesh.mesh_id, mesh_id);
    EXPECT_EQ(mesh.force_lod, 2);

    const auto& materials = dst.GetComponent<MeshMaterialComponent>(loaded_parent).material_overrides;
    ASSERT_EQ(materials.Len(), 2u);
    EXPECT_EQ(materials[0], material_id);
    EXPECT_FALSE(materials[1].IsValid());

    const auto& camera = dst.GetComponent<Camera3dComponent>(loaded_parent);
    EXPECT_DOUBLE_EQ(camera.fov.value, 60.0);
    EXPECT_DOUBLE_EQ(camera.near_plane, 0.5);
    EXPECT_DOUBLE_EQ(camera.far_plane, 500.0);

    // Entity 참조는 dst에 새로 만든 엔티티를 가리켜야 함
    EXPECT_EQ(dst.GetComponent<ParentComponent>(loaded_child_a).parent, loaded_parent);
    EXPECT_EQ(dst.GetComponent<ParentComponent>(loaded_child_b).parent, loaded_parent);
    const auto& children = dst.GetComponent<ChildrenComponent>(loaded_parent).children;
    ASSERT_EQ(children.Len(), 2u);
    EXPECT_EQ(children[0], loaded_child_a);
    EXPECT_EQ(children[1], loaded_child_b);
}

TEST(WorldFileTest, WritesReadableText)
{
    World world;
    world.SpawnEntity(
        TransformComponent{ .position = Vector3(1.0, 2.0, 3.0) },
        Camera3dComponent{ .fov = 60.0_deg }
    );

    const String text = SaveToText(world);

    // 벡터는 인라인 테이블 한 줄로, 각도는 숫자 하나로 씀
    EXPECT_TRUE(text.Contains("format_version = 1")) << text.CStr();
    EXPECT_TRUE(text.Contains("position = { x = 1.0, y = 2.0, z = 3.0 }")) << text.CStr();
    EXPECT_TRUE(text.Contains("fov = 60.0")) << text.CStr();
}

TEST(WorldFileTest, IdsAreStableAcrossSaves)
{
    World world;
    world.SpawnEntity(NameComponent{ .name = "a" });
    world.SpawnEntity(NameComponent{ .name = "b" });
    world.SpawnEntity(NameComponent{ .name = "c" });

    const Array<i64> first_ids = ReadEntityIds(SaveToText(world));
    ASSERT_EQ(first_ids.Len(), 3u);
    for (const i64 id : first_ids)
    {
        EXPECT_GT(id, 0);
    }
    EXPECT_EQ(ReadEntityIds(SaveToText(world)), first_ids);

    // 읽은 월드를 다시 저장해도 파일의 ID를 그대로 씀
    World loaded;
    WorldFileReader reader(loaded);
    LoadFromText(reader, SaveToText(world));
    EXPECT_EQ(ReadEntityIds(SaveToText(loaded)), first_ids);
}

TEST(WorldFileTest, GlobalTransformIsNotSaved)
{
    World src;
    src.SpawnEntity(
        TransformComponent{ .position = Vector3(1.0, 0.0, 0.0) },
        GlobalTransformComponent{ .value = Matrix4x4::Identity() }
    );

    // 영속 ID는 컴포넌트가 아니라 엔티티의 id로 씀
    const String text = SaveToText(src);
    EXPECT_TRUE(text.Contains("se::TransformComponent")) << text.CStr();
    EXPECT_FALSE(text.Contains("GlobalTransformComponent")) << text.CStr();
    EXPECT_FALSE(text.Contains("PersistentIdComponent")) << text.CStr();

    World dst;
    WorldFileReader reader(dst);
    LoadFromText(reader, text);
    ASSERT_EQ(dst.GetAliveEntities().Len(), 1u);

    const Entity entity = dst.GetAliveEntities()[0];
    EXPECT_DOUBLE_EQ(dst.GetComponent<TransformComponent>(entity).position.x, 1.0);
    EXPECT_FALSE(dst.HasComponent<GlobalTransformComponent>(entity));
}

TEST(WorldFileTest, LoadsAdditivelyIntoNonEmptyWorld)
{
    World world;
    const Entity parent = world.SpawnEntity(NameComponent{ .name = "parent" });
    const Entity child = world.SpawnEntity(NameComponent{ .name = "child" }, ParentComponent{ .parent = parent });
    world.AddComponent(parent, ChildrenComponent{ .children = { child } });
    const String text = SaveToText(world);

    // 같은 파일을 다시 읽으면 파일의 ID가 모두 world에 있으므로 새 ID를 붙인 새 엔티티로 더해짐
    WorldFileReader reader(world);
    LoadFromText(reader, text);
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
    ASSERT_EQ(world.GetAliveEntities().Len(), 4u);

    const Entity loaded_parent = FindByName(world, "parent", parent);
    const Entity loaded_child = FindByName(world, "child", child);
    ASSERT_TRUE(loaded_parent.IsValid());
    ASSERT_TRUE(loaded_child.IsValid());

    // 읽은 엔티티의 참조는 원래 엔티티가 아니라 함께 읽은 엔티티를 가리킴
    EXPECT_EQ(world.GetComponent<ParentComponent>(loaded_child).parent, loaded_parent);
    const auto& loaded_children = world.GetComponent<ChildrenComponent>(loaded_parent).children;
    ASSERT_EQ(loaded_children.Len(), 1u);
    EXPECT_EQ(loaded_children[0], loaded_child);
    EXPECT_EQ(world.GetComponent<ParentComponent>(child).parent, parent);

    HashSet<u64> ids;
    for (const Entity entity : world.GetAliveEntities())
    {
        EXPECT_TRUE(ids.Insert(world.GetComponent<PersistentIdComponent>(entity).id));
    }
}

TEST(WorldFileTest, UnknownComponentIsSkippedWithWarning)
{
    constexpr StringView text = R"(
format_version = 1

[[entities]]
id = 7

[entities.components."se::NameComponent"]
name = "kept"

[entities.components."se::RemovedComponent"]
value = 3
)";

    World world;
    WorldFileReader reader(world);
    LoadFromText(reader, text);

    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    const Entity entity = world.GetAliveEntities()[0];
    EXPECT_EQ(world.GetComponent<NameComponent>(entity).name, "kept");
    EXPECT_EQ(world.GetComponent<PersistentIdComponent>(entity).id, 7u);

    ASSERT_EQ(reader.GetWarnings().Len(), 1u);
    EXPECT_TRUE(reader.GetWarnings()[0].Contains("se::RemovedComponent")) << reader.GetWarnings()[0].CStr();
}

TEST(WorldFileTest, UnresolvedReferenceIsReported)
{
    constexpr StringView text = R"(
format_version = 1

[[entities]]
id = 1

[entities.components."se::ParentComponent"]
parent = 999
)";

    World world;
    WorldFileReader reader(world);
    LoadFromText(reader, text);

    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    EXPECT_FALSE(world.GetComponent<ParentComponent>(world.GetAliveEntities()[0]).parent.IsValid());

    ASSERT_EQ(reader.GetWarnings().Len(), 1u);
    const String& warning = reader.GetWarnings()[0];
    EXPECT_TRUE(warning.Contains("se::ParentComponent")) << warning.CStr();
    EXPECT_TRUE(warning.Contains("999")) << warning.CStr();
}

TEST(WorldFileTest, RejectsLegacyBinaryFile)
{
    // 레거시 바이너리 월드 파일은 u32 매직 "SEWD"로 시작함
    constexpr StringView bytes = "SEWD\x01\x00\x00\x00";

    World world;
    WorldFileReader reader(world);
    const auto result = reader.Read(bytes);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("legacy")) << result.Error().CStr();
    EXPECT_TRUE(world.GetAliveEntities().IsEmpty());
}

TEST(WorldFileTest, FailedReadRemovesCreatedEntities)
{
    // 두 번째 엔티티의 name이 문자열이 아니라 읽기에 실패함
    constexpr StringView text = R"(
format_version = 1

[[entities]]
id = 1

[entities.components."se::NameComponent"]
name = "first"

[[entities]]
id = 2

[entities.components."se::NameComponent"]
name = 5
)";

    World world;
    const Entity existing = world.SpawnEntity(NameComponent{ .name = "existing" });

    WorldFileReader reader(world);
    const auto result = reader.Read(text);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("entity 2")) << result.Error().CStr();

    // 읽기 전에 있던 엔티티만 남음
    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    EXPECT_EQ(world.GetAliveEntities()[0], existing);
}

TEST(WorldFileTest, UnregisteredComponentFailsToSave)
{
    // 등록이 없는 컴포넌트를 건너뛰면 파일에서 조용히 사라지므로 저장이 실패해야 함
    World world;
    world.SpawnEntity(NameComponent{ .name = "a" }, se_world_file_test::UnregisteredComponent{ .value = 1 });

    WorldFileWriter writer(world);
    const auto result = writer.Write();
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("not registered")) << result.Error().CStr();
}
