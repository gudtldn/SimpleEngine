#include "gtest/gtest.h"

#include "SimpleEngine/Core/Serialization/MemoryArchive.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/ECS/World.h"
#include "SimpleEngine/ECS/Components/ChildrenComponent.h"
#include "SimpleEngine/ECS/Components/GlobalTransformComponent.h"
#include "SimpleEngine/ECS/Components/NameComponent.h"
#include "SimpleEngine/ECS/Components/ParentComponent.h"
#include "SimpleEngine/ECS/Components/TransformComponent.h"

using namespace se;


namespace
{

// --- Binary round-trip helper ---
void BinaryRoundTrip(World& src, World& dst)
{
    Array<uint8> buffer;
    {
        MemoryWriter writer(buffer);
        writer << src;
    }
    {
        MemoryReader reader(buffer);
        reader << dst;
    }
}

// --- TOML round-trip helper ---
void TomlRoundTrip(World& src, World& dst)
{
    toml::table tbl;
    {
        TomlWriter writer(tbl);
        writer << src;
    }
    {
        TomlReader reader(tbl);
        reader << dst;
    }
}

} // namespace


class WorldSerializationTest : public ::testing::Test
{
};

// 1) 빈 World 라운드트립
TEST_F(WorldSerializationTest, EmptyWorld_Binary)
{
    World src;
    World dst;
    BinaryRoundTrip(src, dst);

    EXPECT_TRUE(dst.GetAliveEntities().IsEmpty());
}

TEST_F(WorldSerializationTest, EmptyWorld_Toml)
{
    World src;
    World dst;
    TomlRoundTrip(src, dst);

    EXPECT_TRUE(dst.GetAliveEntities().IsEmpty());
}

// 2) 엔티티 + 컴포넌트 기본 라운드트립 (Binary)
TEST_F(WorldSerializationTest, BasicRoundTrip_Binary)
{
    World src;

    Entity e1 = src.SpawnEntity(
        NameComponent{ .name = "Entity_A" },
        TransformComponent{
            .position = Vector3(1.0, 2.0, 3.0),
            .scale = Vector3(2.0, 2.0, 2.0),
        }
    );

    Entity e2 = src.SpawnEntity(
        NameComponent{ .name = "Entity_B" },
        TransformComponent{
            .position = Vector3(4.0, 5.0, 6.0),
        }
    );

    World dst;
    BinaryRoundTrip(src, dst);

    // 엔티티 수 검증
    EXPECT_EQ(dst.GetAliveEntities().Len(), 2u);
    EXPECT_TRUE(dst.IsEntityAlive(e1));
    EXPECT_TRUE(dst.IsEntityAlive(e2));

    // 컴포넌트 값 검증
    auto& name1 = dst.GetComponent<NameComponent>(e1);
    EXPECT_EQ(name1.name, "Entity_A");

    auto& t1 = dst.GetComponent<TransformComponent>(e1);
    EXPECT_DOUBLE_EQ(t1.position.x, 1.0);
    EXPECT_DOUBLE_EQ(t1.position.y, 2.0);
    EXPECT_DOUBLE_EQ(t1.position.z, 3.0);
    EXPECT_DOUBLE_EQ(t1.scale.x, 2.0);

    auto& name2 = dst.GetComponent<NameComponent>(e2);
    EXPECT_EQ(name2.name, "Entity_B");

    auto& t2 = dst.GetComponent<TransformComponent>(e2);
    EXPECT_DOUBLE_EQ(t2.position.x, 4.0);
}

// 3) 엔티티 + 컴포넌트 기본 라운드트립 (TOML)
TEST_F(WorldSerializationTest, BasicRoundTrip_Toml)
{
    World src;

    Entity e1 = src.SpawnEntity(
        NameComponent{ .name = "Entity_A" },
        TransformComponent{
            .position = Vector3(1.0, 2.0, 3.0),
        }
    );

    Entity e2 = src.SpawnEntity(
        NameComponent{ .name = "Entity_B" },
        TransformComponent{
            .position = Vector3(4.0, 5.0, 6.0),
        }
    );

    World dst;
    TomlRoundTrip(src, dst);

    EXPECT_EQ(dst.GetAliveEntities().Len(), 2u);
    EXPECT_TRUE(dst.IsEntityAlive(e1));
    EXPECT_TRUE(dst.IsEntityAlive(e2));

    auto& name1 = dst.GetComponent<NameComponent>(e1);
    EXPECT_EQ(name1.name, "Entity_A");

    auto& t1 = dst.GetComponent<TransformComponent>(e1);
    EXPECT_DOUBLE_EQ(t1.position.x, 1.0);
    EXPECT_DOUBLE_EQ(t1.position.y, 2.0);
    EXPECT_DOUBLE_EQ(t1.position.z, 3.0);

    auto& name2 = dst.GetComponent<NameComponent>(e2);
    EXPECT_EQ(name2.name, "Entity_B");

    auto& t2 = dst.GetComponent<TransformComponent>(e2);
    EXPECT_DOUBLE_EQ(t2.position.x, 4.0);
    EXPECT_DOUBLE_EQ(t2.position.y, 5.0);
    EXPECT_DOUBLE_EQ(t2.position.z, 6.0);
}

// 3-1) TOML 다수 엔티티 + Destroy/재사용 (EntityManager 상태 보존)
TEST_F(WorldSerializationTest, EntityManagerStatePreservation_Toml)
{
    World src;

    Entity e1 = src.SpawnEntity(NameComponent{ .name = "first" });
    Entity e2 = src.SpawnEntity(NameComponent{ .name = "second" });
    Entity e3 = src.SpawnEntity(NameComponent{ .name = "third" });

    // e2를 삭제 -> free_ids에 e2 슬롯 추가, generation 증가
    src.DestroyEntity(e2);

    // 새 엔티티 생성 -> e2의 슬롯을 재사용하되 generation이 다름
    Entity e4 = src.SpawnEntity(NameComponent{ .name = "fourth" });
    EXPECT_EQ(e4.GetId(), e2.GetId());
    EXPECT_NE(e4.GetGeneration(), e2.GetGeneration());

    World dst;
    TomlRoundTrip(src, dst);

    EXPECT_TRUE(dst.IsEntityAlive(e1));
    EXPECT_FALSE(dst.IsEntityAlive(e2));
    EXPECT_TRUE(dst.IsEntityAlive(e3));
    EXPECT_TRUE(dst.IsEntityAlive(e4));

    EXPECT_EQ(dst.GetComponent<NameComponent>(e1).name, "first");
    EXPECT_EQ(dst.GetComponent<NameComponent>(e3).name, "third");
    EXPECT_EQ(dst.GetComponent<NameComponent>(e4).name, "fourth");
}

// 3-2) TOML 부모-자식 참조 무결성 (다수 Entity 참조)
TEST_F(WorldSerializationTest, ParentChildRelationship_Toml)
{
    World src;

    Entity parent = src.SpawnEntity(NameComponent{ .name = "parent" });
    Entity child1 = src.SpawnEntity(NameComponent{ .name = "child1" });
    Entity child2 = src.SpawnEntity(NameComponent{ .name = "child2" });

    src.AddComponent<ParentComponent>(child1, ParentComponent{ .parent = parent });
    src.AddComponent<ParentComponent>(child2, ParentComponent{ .parent = parent });
    src.AddComponent<ChildrenComponent>(parent, ChildrenComponent{ .children = { child1, child2 } });

    World dst;
    TomlRoundTrip(src, dst);

    auto& p1 = dst.GetComponent<ParentComponent>(child1);
    EXPECT_EQ(p1.parent, parent);

    auto& p2 = dst.GetComponent<ParentComponent>(child2);
    EXPECT_EQ(p2.parent, parent);

    auto& ch = dst.GetComponent<ChildrenComponent>(parent);
    EXPECT_EQ(ch.children.Len(), 2u);
    EXPECT_TRUE(ch.children.Contains(child1));
    EXPECT_TRUE(ch.children.Contains(child2));
}

// 4) EntityManager 상태 보존 (생성 -> 삭제 -> 재생성 후 generation/free_ids 검증)
TEST_F(WorldSerializationTest, EntityManagerStatePreservation_Binary)
{
    World src;

    Entity e1 = src.SpawnEntity(NameComponent{ .name = "first" });
    Entity e2 = src.SpawnEntity(NameComponent{ .name = "second" });
    Entity e3 = src.SpawnEntity(NameComponent{ .name = "third" });

    // e2를 삭제 -> free_ids에 e2 슬롯 추가, generation 증가
    src.DestroyEntity(e2);

    // 새 엔티티 생성 -> e2의 슬롯을 재사용하되 generation이 다름
    Entity e4 = src.SpawnEntity(NameComponent{ .name = "fourth" });
    EXPECT_EQ(e4.GetId(), e2.GetId());            // 같은 슬롯
    EXPECT_NE(e4.GetGeneration(), e2.GetGeneration()); // 다른 세대

    World dst;
    BinaryRoundTrip(src, dst);

    // e1, e3, e4는 살아있고 e2는 죽어있어야 함
    EXPECT_TRUE(dst.IsEntityAlive(e1));
    EXPECT_FALSE(dst.IsEntityAlive(e2));
    EXPECT_TRUE(dst.IsEntityAlive(e3));
    EXPECT_TRUE(dst.IsEntityAlive(e4));

    EXPECT_EQ(dst.GetComponent<NameComponent>(e4).name, "fourth");
}

// 5) 부모-자식 관계 (Entity 참조 무결성)
TEST_F(WorldSerializationTest, ParentChildRelationship_Binary)
{
    World src;

    Entity parent = src.SpawnEntity(NameComponent{ .name = "parent" });
    Entity child1 = src.SpawnEntity(NameComponent{ .name = "child1" });
    Entity child2 = src.SpawnEntity(NameComponent{ .name = "child2" });

    // 부모-자식 관계 수동 설정
    src.AddComponent<ParentComponent>(child1, ParentComponent{ .parent = parent });
    src.AddComponent<ParentComponent>(child2, ParentComponent{ .parent = parent });
    src.AddComponent<ChildrenComponent>(parent, ChildrenComponent{ .children = { child1, child2 } });

    World dst;
    BinaryRoundTrip(src, dst);

    // Entity 참조가 ID 보존으로 그대로 유효한지 검증
    auto& p1 = dst.GetComponent<ParentComponent>(child1);
    EXPECT_EQ(p1.parent, parent);

    auto& p2 = dst.GetComponent<ParentComponent>(child2);
    EXPECT_EQ(p2.parent, parent);

    auto& ch = dst.GetComponent<ChildrenComponent>(parent);
    EXPECT_EQ(ch.children.Len(), 2u);
    EXPECT_TRUE(ch.children.Contains(child1));
    EXPECT_TRUE(ch.children.Contains(child2));
}

// 6) Transient 컴포넌트 필터링 (GlobalTransformComponent 제외 확인)
TEST_F(WorldSerializationTest, TransientComponentFiltered_Binary)
{
    World src;

    Entity e = src.SpawnEntity(
        TransformComponent{ .position = Vector3(1.0, 0.0, 0.0) },
        GlobalTransformComponent{ .value = Matrix4x4::Identity() }
    );

    // 직렬화 전에 GlobalTransformComponent이 있는지 확인
    EXPECT_TRUE(src.HasComponent<GlobalTransformComponent>(e));

    World dst;
    BinaryRoundTrip(src, dst);

    // TransformComponent은 복원되어야 함
    EXPECT_TRUE(dst.HasComponent<TransformComponent>(e));
    EXPECT_DOUBLE_EQ(dst.GetComponent<TransformComponent>(e).position.x, 1.0);

    // GlobalTransformComponent은 Transient이므로 복원되지 않아야 함
    EXPECT_FALSE(dst.HasComponent<GlobalTransformComponent>(e));
}
