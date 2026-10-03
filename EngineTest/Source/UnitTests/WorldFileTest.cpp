#include "gtest/gtest.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Serialization/BinaryArchive.h"
#include "SimpleEngine/Core/Serialization/JsonArchive.h"
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
/** world를 월드 파일 JSON 텍스트로 씁니다. 실패하면 테스트를 실패로 표시하고 빈 문자열을 돌려줍니다. */
[[nodiscard]] String SaveToText(World& world)
{
    WorldFileWriter writer(world);
    JsonWriter json_writer;
    if (const auto result = writer.Write(json_writer); result.HasError())
    {
        ADD_FAILURE() << result.Error().CStr();
        return {};
    }

    auto text = json_writer.ToText();
    if (text.HasError())
    {
        ADD_FAILURE() << text.Error().CStr();
        return {};
    }
    return std::move(text).Value();
}

/** reader로 JSON text를 읽습니다. 실패하면 테스트를 실패로 표시합니다. */
void LoadFromText(WorldFileReader& reader, StringView text)
{
    JsonReader json_reader{ text };
    if (const auto result = reader.Read(json_reader); result.HasError())
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
[[nodiscard]] Array<u64> ReadEntityIds(StringView text)
{
    Array<u64> ids;
    JsonReader reader{ text };
    reader.BeginStruct();
    if (reader.Field("entities"))
    {
        u64 count = 0;
        reader.BeginSeq(count);
        for (u64 index = 0; index < count && !reader.HasError(); ++index)
        {
            reader.BeginStruct();
            i64 id = 0;
            if (reader.Field("id"))
            {
                reader.Int(id, EIntWidth::Bits64, false);
            }
            if (reader.Field("components"))
            {
                reader.SkipSection();
            }
            reader.EndStruct();
            ids.Push(static_cast<u64>(id));
        }
        reader.EndSeq();
    }
    reader.EndStruct();

    if (reader.HasError())
    {
        ADD_FAILURE() << String{ reader.GetError() }.CStr();
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

TEST(WorldFileTest, BinaryRoundTripInMemory)
{
    // 구간과 Rewind가 바이너리 포맷에서도 동작하는지 확인
    World src;
    const Entity parent = src.SpawnEntity(NameComponent{ .name = "parent" }, TransformComponent{ .position = Vector3(1.0, 2.0, 3.0) });
    const Entity child = src.SpawnEntity(NameComponent{ .name = "child" }, ParentComponent{ .parent = parent });
    src.AddComponent(parent, ChildrenComponent{ .children = { child } });

    Array<u8> buffer;
    {
        BinaryWriter writer(buffer);
        WorldFileWriter world_writer(src);
        const auto result = world_writer.Write(writer);
        ASSERT_TRUE(result.HasValue()) << result.Error().CStr();
        EXPECT_TRUE(world_writer.GetWarnings().IsEmpty());
    }

    World dst;
    WorldFileReader world_reader(dst);
    BinaryReader reader(buffer);
    const auto result = world_reader.Read(reader);
    ASSERT_TRUE(result.HasValue()) << result.Error().CStr();
    EXPECT_TRUE(world_reader.GetWarnings().IsEmpty());
    ASSERT_EQ(dst.GetAliveEntities().Len(), 2u);

    const Entity loaded_parent = FindByName(dst, "parent");
    const Entity loaded_child = FindByName(dst, "child");
    ASSERT_TRUE(loaded_parent.IsValid());
    ASSERT_TRUE(loaded_child.IsValid());
    EXPECT_DOUBLE_EQ(dst.GetComponent<TransformComponent>(loaded_parent).position.y, 2.0);
    EXPECT_EQ(dst.GetComponent<ParentComponent>(loaded_child).parent, loaded_parent);
    EXPECT_EQ(dst.GetComponent<ChildrenComponent>(loaded_parent).children, (Array<Entity>{ loaded_child }));
    EXPECT_EQ(dst.GetComponent<PersistentIdComponent>(loaded_parent).id, src.GetComponent<PersistentIdComponent>(parent).id);
}

TEST(WorldFileTest, WritesReadableText)
{
    World world;
    world.SpawnEntity(
        TransformComponent{ .position = Vector3(1.0, 2.0, 3.0) },
        Camera3dComponent{ .fov = 60.0_deg }
    );

    const String text = SaveToText(world);

    // 벡터는 한 줄 객체로, 각도는 숫자 하나로 씀
    EXPECT_TRUE(text.Contains("\"format_version\": 1")) << text.CStr();
    EXPECT_TRUE(text.Contains("\"position\": { \"x\": 1.0, \"y\": 2.0, \"z\": 3.0 }")) << text.CStr();
    EXPECT_TRUE(text.Contains("\"fov\": 60.0")) << text.CStr();
}

TEST(WorldFileTest, IdsAreStableAcrossSaves)
{
    World world;
    world.SpawnEntity(NameComponent{ .name = "a" });
    world.SpawnEntity(NameComponent{ .name = "b" });
    world.SpawnEntity(NameComponent{ .name = "c" });

    const Array<u64> first_ids = ReadEntityIds(SaveToText(world));
    ASSERT_EQ(first_ids.Len(), 3u);
    for (const u64 id : first_ids)
    {
        EXPECT_GT(id, 0u);
    }
    EXPECT_EQ(ReadEntityIds(SaveToText(world)), first_ids);

    // 읽은 월드를 다시 저장해도 파일의 ID를 그대로 씀
    World loaded;
    WorldFileReader reader(loaded);
    LoadFromText(reader, SaveToText(world));
    EXPECT_EQ(ReadEntityIds(SaveToText(loaded)), first_ids);
}

TEST(WorldFileTest, NewIdsAreBelowTwoToThe53)
{
    // JSON 숫자로 정확히 쓸 수 있는 범위
    World world;
    for (usize index = 0; index < 64; ++index)
    {
        world.SpawnEntity(NameComponent{ .name = "entity" });
    }

    const Array<u64> ids = ReadEntityIds(SaveToText(world));
    ASSERT_EQ(ids.Len(), 64u);
    for (const u64 id : ids)
    {
        EXPECT_GT(id, 0u);
        EXPECT_LT(id, u64{ 1 } << 53);
    }
}

TEST(WorldFileTest, GlobalTransformIsNotSaved)
{
    World src;
    src.SpawnEntity(
        TransformComponent{ .position = Vector3(1.0, 0.0, 0.0) },
        GlobalTransformComponent{ .value = Matrix4x4::Identity() }
    );

    const String text = SaveToText(src);
    EXPECT_TRUE(text.Contains("se::TransformComponent")) << text.CStr();
    EXPECT_FALSE(text.Contains("GlobalTransformComponent")) << text.CStr();

    World dst;
    WorldFileReader reader(dst);
    LoadFromText(reader, text);
    ASSERT_EQ(dst.GetAliveEntities().Len(), 1u);

    const Entity entity = dst.GetAliveEntities()[0];
    EXPECT_DOUBLE_EQ(dst.GetComponent<TransformComponent>(entity).position.x, 1.0);
    EXPECT_FALSE(dst.HasComponent<GlobalTransformComponent>(entity));
}

TEST(WorldFileTest, PersistentIdIsNotSavedAsComponent)
{
    // 영속 ID는 컴포넌트가 아니라 엔티티의 id로 씀
    World world;
    world.SpawnEntity(NameComponent{ .name = "a" });

    const String text = SaveToText(world);
    EXPECT_FALSE(text.Contains("PersistentIdComponent")) << text.CStr();
    EXPECT_EQ(ReadEntityIds(text).Len(), 1u);
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

TEST(WorldFileTest, ForwardReferenceIsResolved)
{
    // 첫 엔티티가 뒤에 나오는 엔티티를 가리킴
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 1, "components": { "se::NameComponent": { "name": "child" }, "se::ParentComponent": { "parent": 2 } } },
        { "id": 2, "components": { "se::NameComponent": { "name": "parent" } } }
    ]
})";

    World world;
    WorldFileReader reader(world);
    LoadFromText(reader, text);
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
    ASSERT_EQ(world.GetAliveEntities().Len(), 2u);

    const Entity child = FindByName(world, "child");
    const Entity parent = FindByName(world, "parent");
    ASSERT_TRUE(child.IsValid());
    ASSERT_TRUE(parent.IsValid());
    EXPECT_EQ(world.GetComponent<ParentComponent>(child).parent, parent);
}

TEST(WorldFileTest, UnknownComponentIsSkippedWithWarning)
{
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 7, "components": { "se::NameComponent": { "name": "kept" }, "se::RemovedComponent": { "value": 3 } } }
    ]
})";

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

TEST(WorldFileTest, TransientComponentIsSkippedWithWarning)
{
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 1, "components": { "se::GlobalTransformComponent": {} } }
    ]
})";

    World world;
    WorldFileReader reader(world);
    LoadFromText(reader, text);

    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    EXPECT_FALSE(world.HasComponent<GlobalTransformComponent>(world.GetAliveEntities()[0]));
    ASSERT_EQ(reader.GetWarnings().Len(), 1u);
    EXPECT_TRUE(reader.GetWarnings()[0].Contains("se::GlobalTransformComponent")) << reader.GetWarnings()[0].CStr();
}

TEST(WorldFileTest, UnknownKeyOutsideComponentsIsWarned)
{
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 1, "tag": 5, "components": {} }
    ]
})";

    World world;
    WorldFileReader reader(world);
    LoadFromText(reader, text);

    // 컴포넌트 밖의 경고는 엔티티와 컴포넌트 접두어 없이 문서 위치만 남김
    ASSERT_EQ(reader.GetWarnings().Len(), 1u);
    const String& warning = reader.GetWarnings()[0];
    EXPECT_TRUE(warning.Contains("entities[0].tag")) << warning.CStr();
    EXPECT_FALSE(warning.Contains("entity 1")) << warning.CStr();
}

TEST(WorldFileTest, UnresolvedReferenceIsReported)
{
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 1, "components": { "se::ParentComponent": { "parent": 999 } } }
    ]
})";

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

TEST(WorldFileTest, DuplicateIdIsError)
{
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 5, "components": { "se::NameComponent": { "name": "first" } } },
        { "id": 5, "components": { "se::NameComponent": { "name": "second" } } }
    ]
})";

    World world;
    WorldFileReader reader(world);
    JsonReader json_reader{ text };
    const auto result = reader.Read(json_reader);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("more than once")) << result.Error().CStr();
    EXPECT_TRUE(world.GetAliveEntities().IsEmpty());
}

TEST(WorldFileTest, MissingFormatVersionIsError)
{
    World world;
    WorldFileReader reader(world);
    JsonReader json_reader{ R"({ "entities": [] })" };
    const auto result = reader.Read(json_reader);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("format_version")) << result.Error().CStr();
}

TEST(WorldFileTest, UnsupportedFormatVersionIsError)
{
    World world;
    WorldFileReader reader(world);
    JsonReader json_reader{ R"({ "format_version": 2, "entities": [] })" };
    const auto result = reader.Read(json_reader);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("format_version 2 is not supported")) << result.Error().CStr();
}

TEST(WorldFileTest, FailedReadRemovesCreatedEntities)
{
    // 두 번째 엔티티의 name이 문자열이 아니라 읽기에 실패함
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 1, "components": { "se::NameComponent": { "name": "first" } } },
        { "id": 2, "components": { "se::NameComponent": { "name": 5 } } }
    ]
})";

    World world;
    const Entity existing = world.SpawnEntity(NameComponent{ .name = "existing" });

    WorldFileReader reader(world);
    JsonReader json_reader{ text };
    const auto result = reader.Read(json_reader);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("entity 2")) << result.Error().CStr();

    // 읽기 전에 있던 엔티티만 남음
    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    EXPECT_EQ(world.GetAliveEntities()[0], existing);
}

TEST(WorldFileTest, BinaryUnknownComponentIsError)
{
    // 바이너리는 값의 길이를 알 수 없어 모르는 컴포넌트를 건너뛸 수 없음
    Array<u8> buffer;
    {
        BinaryWriter writer(buffer);
        writer.BeginStruct();
        writer.Field("format_version");
        writer.Int(1, EIntWidth::Bits32, true);
        writer.Field("entities");
        writer.BeginSeq(1, ESeqOrder::Ordered);
        writer.BeginStruct();
        writer.Field("id");
        writer.Int(1, EIntWidth::Bits64, false);
        writer.Field("components");
        writer.BeginSection();
        writer.BeginMap(1);
        writer.BeginMapEntry();
        writer.Str("se::RemovedComponent");
        writer.Int(3, EIntWidth::Bits32, true);
        writer.EndMapEntry();
        writer.EndMap();
        writer.EndSection();
        writer.EndStruct();
        writer.EndSeq();
        writer.EndStruct();
        ASSERT_FALSE(writer.HasError()) << String{ writer.GetError() }.CStr();
    }

    World world;
    WorldFileReader reader(world);
    BinaryReader binary_reader(buffer);
    const auto result = reader.Read(binary_reader);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("se::RemovedComponent")) << result.Error().CStr();
    EXPECT_TRUE(result.Error().Contains("binary")) << result.Error().CStr();
    EXPECT_TRUE(world.GetAliveEntities().IsEmpty());
}

TEST(WorldFileTest, UnregisteredComponentFailsToSave)
{
    // 등록이 없는 컴포넌트를 건너뛰면 파일에서 조용히 사라지므로 저장이 실패해야 함
    World world;
    world.SpawnEntity(NameComponent{ .name = "a" }, se_world_file_test::UnregisteredComponent{ .value = 1 });

    WorldFileWriter writer(world);
    JsonWriter json_writer;
    const auto result = writer.Write(json_writer);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("not registered")) << result.Error().CStr();
}

TEST(WorldFileTest, DanglingReferenceIsWrittenAsNullWithWarning)
{
    // 저장하지 않는 엔티티를 가리키는 참조는 null로 쓰고, 같은 엔티티를 가리켜도 컴포넌트마다 경고로 남김
    World world;
    const Entity removed = world.SpawnEntity(NameComponent{ .name = "removed" });
    world.SpawnEntity(NameComponent{ .name = "child" }, ParentComponent{ .parent = removed });
    world.SpawnEntity(NameComponent{ .name = "other child" }, ParentComponent{ .parent = removed });
    world.DestroyEntity(removed);

    WorldFileWriter writer(world);
    JsonWriter json_writer;
    const auto result = writer.Write(json_writer);
    ASSERT_TRUE(result.HasValue()) << result.Error().CStr();

    ASSERT_EQ(writer.GetWarnings().Len(), 2u);
    for (const String& warning : writer.GetWarnings())
    {
        EXPECT_TRUE(warning.Contains("se::ParentComponent")) << warning.CStr();
        EXPECT_TRUE(warning.Contains("written as null")) << warning.CStr();
    }

    const auto text = json_writer.ToText();
    ASSERT_TRUE(text.HasValue());
    EXPECT_TRUE(text.Value().Contains("\"parent\": 0")) << text.Value().CStr();

    // 다시 읽으면 null 참조로 돌아오고 경고가 없음
    World loaded;
    WorldFileReader reader(loaded);
    LoadFromText(reader, text.Value());
    EXPECT_TRUE(reader.GetWarnings().IsEmpty());
    const Entity loaded_child = FindByName(loaded, "child");
    ASSERT_TRUE(loaded_child.IsValid());
    EXPECT_FALSE(loaded.GetComponent<ParentComponent>(loaded_child).parent.IsValid());
}

TEST(WorldFileTest, ReplaceSwapsWorldContentsAndKeepsFileIds)
{
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 42, "components": { "se::NameComponent": { "name": "loaded" }, "se::RemovedComponent": { "value": 3 } } }
    ]
})";

    World world;
    world.SpawnEntity(NameComponent{ .name = "old_a" });
    world.SpawnEntity(NameComponent{ .name = "old_b" });

    WorldFileReader reader(world);
    JsonReader json_reader{ text };
    const auto result = reader.Replace(json_reader);
    ASSERT_TRUE(result.HasValue()) << result.Error().CStr();

    // 기존 엔티티는 사라지고 파일의 엔티티가 파일의 영속 ID 그대로 남음
    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    const Entity entity = world.GetAliveEntities()[0];
    EXPECT_EQ(world.GetComponent<NameComponent>(entity).name, "loaded");
    EXPECT_EQ(world.GetComponent<PersistentIdComponent>(entity).id, 42u);

    // 경고는 마지막으로 읽은 쪽의 것만 남음
    ASSERT_EQ(reader.GetWarnings().Len(), 1u);
    EXPECT_TRUE(reader.GetWarnings()[0].Contains("se::RemovedComponent")) << reader.GetWarnings()[0].CStr();
}

TEST(WorldFileTest, ReplaceKeepsWorldWhenComponentReadFails)
{
    // 두 번째 엔티티의 name이 문자열이 아니라 읽기에 실패함
    constexpr StringView text = R"({
    "format_version": 1,
    "entities": [
        { "id": 1, "components": { "se::NameComponent": { "name": "first" } } },
        { "id": 2, "components": { "se::NameComponent": { "name": 5 } } }
    ]
})";

    World world;
    const Entity existing = world.SpawnEntity(NameComponent{ .name = "existing" }, PersistentIdComponent{ .id = 77 });

    WorldFileReader reader(world);
    JsonReader json_reader{ text };
    const auto result = reader.Replace(json_reader);
    ASSERT_TRUE(result.HasError());
    EXPECT_TRUE(result.Error().Contains("entity 2")) << result.Error().CStr();

    // 기존 엔티티와 컴포넌트가 그대로 남음
    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    EXPECT_EQ(world.GetAliveEntities()[0], existing);
    EXPECT_EQ(world.GetComponent<NameComponent>(existing).name, "existing");
    EXPECT_EQ(world.GetComponent<PersistentIdComponent>(existing).id, 77u);
}

TEST(WorldFileTest, ReplaceKeepsWorldWhenTextIsNotJson)
{
    World world;
    const Entity existing = world.SpawnEntity(NameComponent{ .name = "existing" });

    WorldFileReader reader(world);
    JsonReader json_reader{ "{ not json" };
    const auto result = reader.Replace(json_reader);
    ASSERT_TRUE(result.HasError());

    ASSERT_EQ(world.GetAliveEntities().Len(), 1u);
    EXPECT_EQ(world.GetAliveEntities()[0], existing);
}
