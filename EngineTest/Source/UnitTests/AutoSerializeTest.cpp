#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Math/MathSerialize.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/AutoSerialize.h"
#include "SimpleEngine/Core/Serialization/MemoryArchive.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Graphics/MeshPrimitives.h"

#include <limits>


using namespace se;
using namespace se::math;
using namespace se::graphics;

// ============================================================================
//  Helper: Write -> Read 라운드트립 유틸
// ============================================================================
namespace
{
template <typename T>
T RoundTrip(const T& original)
{
    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    T copy = original;
    writer << copy;

    MemoryReader reader(buffer);
    T loaded{};
    reader << loaded;
    return loaded;
}

template <typename T>
T RoundTripViaTypeInfo(const T& original)
{
    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<T>());
    EXPECT_NE(info.serialize, nullptr);

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    T copy = original;
    info.serialize(writer, &copy);

    MemoryReader reader(buffer);
    T loaded{};
    info.serialize(reader, &loaded);
    return loaded;
}
} // namespace

// ============================================================================
//  AutoSerialize 테스트용 타입 정의
// ============================================================================
namespace autoserialize_test
{
// 단순 구조체
struct SE_ANNOTATION(=meta::SerializeOnly) SimpleData
{
    SE_ANNOTATION(=meta::Property)
    int32 x = 0;

    SE_ANNOTATION(=meta::Property)
    float y = 0.0f;

    SE_ANNOTATION(=meta::Property)
    String name;

    bool operator==(const SimpleData&) const = default;
};

// 상속 테스트용 Base
struct SE_ANNOTATION(=meta::SerializeOnly) BaseData
{
    SE_ANNOTATION(=meta::Property)
    int32 base_val = 0;

    SE_ANNOTATION(=meta::Property)
    String base_name;

    bool operator==(const BaseData&) const = default;
};

// 상속 테스트용 Derived
struct SE_ANNOTATION(=meta::SerializeOnly) DerivedData : BaseData
{
    using Super = BaseData;

    SE_ANNOTATION(=meta::Property)
    float derived_val = 0.0f;

    SE_ANNOTATION(=meta::Property)
    int32 derived_extra = 0;

    bool operator==(const DerivedData& other) const
    {
        return BaseData::operator==(other)
            && derived_val == other.derived_val
            && derived_extra == other.derived_extra;
    }
};

// Transient 프로퍼티 테스트용
struct SE_ANNOTATION(=meta::SerializeOnly) TransientData
{
    SE_ANNOTATION(=meta::Property)
    int32 saved_val = 0;

    SE_ANNOTATION(=meta::Property, =meta::Transient)
    int32 transient_val = 0;  // Transient -> 직렬화에서 제외

    bool operator==(const TransientData&) const = default;
};

// 빈 구조체 (프로퍼티 없음)
struct SE_ANNOTATION(=meta::SerializeOnly) EmptyReflected
{
    bool operator==(const EmptyReflected&) const = default;
};

// 컨테이너 프로퍼티를 가진 구조체
struct SE_ANNOTATION(=meta::SerializeOnly) ContainerData
{
    SE_ANNOTATION(=meta::Property)
    Array<int32> numbers;

    SE_ANNOTATION(=meta::Property)
    HashMap<String, float> scores;

    bool operator==(const ContainerData&) const = default;
};

// Enum 테스트용
enum class ETestColor : uint8
{
    Red = 0,
    Green = 1,
    Blue = 2,
    Alpha = 255,
};
} // namespace autoserialize_test

// ============================================================================
//  리플렉션 등록
// ============================================================================
using namespace autoserialize_test;

SE_DECLARE_REFLECTION(SimpleData)
SE_BEGIN_REFLECT(SimpleData, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(x, meta::Property)
    SE_REFLECT_PROPERTY(y, meta::Property)
    SE_REFLECT_PROPERTY(name, meta::Property)
SE_END_REFLECT(SimpleData)

SE_DECLARE_REFLECTION(BaseData)
SE_BEGIN_REFLECT(BaseData, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(base_val, meta::Property)
    SE_REFLECT_PROPERTY(base_name, meta::Property)
SE_END_REFLECT(BaseData)

SE_DECLARE_REFLECTION(DerivedData)
SE_BEGIN_REFLECT(DerivedData, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(derived_val, meta::Property)
    SE_REFLECT_PROPERTY(derived_extra, meta::Property)
SE_END_REFLECT(DerivedData)

SE_DECLARE_REFLECTION(TransientData)
SE_BEGIN_REFLECT(TransientData, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(saved_val, meta::Property)
    SE_REFLECT_PROPERTY(transient_val, meta::Transient)
SE_END_REFLECT(TransientData)

SE_DECLARE_REFLECTION(EmptyReflected)
SE_BEGIN_REFLECT(EmptyReflected, meta::SerializeOnly)
SE_END_REFLECT(EmptyReflected)

SE_DECLARE_REFLECTION(ContainerData)
SE_BEGIN_REFLECT(ContainerData, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(numbers, meta::Property)
    SE_REFLECT_PROPERTY(scores, meta::Property)
SE_END_REFLECT(ContainerData)

SE_REFLECT_ENUM(ETestColor)

// ============================================================================
//  Test Fixture
// ============================================================================
class AutoSerializeTest : public ::testing::Test
{
};

// ============================================================================
//  일반적인 TC (Normal Cases)
// ============================================================================

// --- AutoSerialize: 단순 구조체 ---
TEST_F(AutoSerializeTest, SimpleStruct)
{
    SimpleData original{ .x = 42, .y = 3.14f, .name = "Hello" };
    SimpleData loaded = RoundTripViaTypeInfo(original);
    EXPECT_EQ(loaded, original);
}

// --- AutoSerialize: 상속 (Base + Derived) ---
TEST_F(AutoSerializeTest, Inheritance)
{
    DerivedData original;
    original.base_val = 100;
    original.base_name = "Parent";
    original.derived_val = 12.718f;
    original.derived_extra = -42;

    DerivedData loaded = RoundTripViaTypeInfo(original);
    EXPECT_EQ(loaded, original);

    // 부모 프로퍼티가 올바르게 직렬화되었는지 개별 검증
    EXPECT_EQ(loaded.base_val, 100);
    EXPECT_EQ(loaded.base_name, "Parent");
    EXPECT_FLOAT_EQ(loaded.derived_val, 12.718f);
    EXPECT_EQ(loaded.derived_extra, -42);
}

// --- AutoSerialize: 컨테이너 프로퍼티 ---
TEST_F(AutoSerializeTest, ContainerProperties)
{
    ContainerData original;
    original.numbers = { 1, 2, 3, 4, 5 };
    original.scores.Insert("Alice", 95.5f);
    original.scores.Insert("Bob", 87.3f);

    ContainerData loaded = RoundTripViaTypeInfo(original);
    EXPECT_EQ(loaded, original);
}

// --- AssetId 라운드트립 ---
TEST_F(AutoSerializeTest, AssetId_RoundTrip)
{
    Guid guid = Guid::NewGuid();
    AssetId original(guid);

    AssetId loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
    EXPECT_EQ(loaded.GetGuid(), guid);
}

// --- Entity 라운드트립 ---
TEST_F(AutoSerializeTest, Entity_RoundTrip)
{
    // Entity의 private 생성자를 사용할 수 없으므로,
    // Serialize를 통한 쓰기/읽기 순환을 직접 테스트
    // 먼저 유효한 Entity를 binary로 생성
    Array<uint8> buffer;
    {
        MemoryWriter writer(buffer);
        uint32 id = 42;
        uint32 gen = 7;
        writer << id << gen;
    }

    // Serialize(ar, entity) 경로로 로드
    Entity loaded;
    {
        MemoryReader reader(buffer);
        reader << loaded;
    }
    EXPECT_EQ(loaded.GetId(), 42u);
    EXPECT_EQ(loaded.GetGeneration(), 7u);
    EXPECT_TRUE(loaded.IsValid());

    // 다시 저장 -> 재로드 라운드트립
    Entity reloaded = RoundTrip(loaded);
    EXPECT_EQ(reloaded, loaded);
}

// --- Math: Vector3 ---
TEST_F(AutoSerializeTest, Math_Vector3)
{
    Vector3f original{ 1.0f, -2.5f, 3.14f };
    Vector3f loaded = RoundTrip(original);
    EXPECT_FLOAT_EQ(loaded.x, original.x);
    EXPECT_FLOAT_EQ(loaded.y, original.y);
    EXPECT_FLOAT_EQ(loaded.z, original.z);
}

// --- Math: Quaternion ---
TEST_F(AutoSerializeTest, Math_Quaternion)
{
    Quaternionf original{ 0.1f, 0.2f, 0.3f, 0.9f };
    Quaternionf loaded = RoundTrip(original);
    EXPECT_FLOAT_EQ(loaded.x, original.x);
    EXPECT_FLOAT_EQ(loaded.y, original.y);
    EXPECT_FLOAT_EQ(loaded.z, original.z);
    EXPECT_FLOAT_EQ(loaded.w, original.w);
}

// --- Math: Rotator ---
TEST_F(AutoSerializeTest, Math_Rotator)
{
    Rotatorf original{ 45.0_degf, 90.0_degf, -30.0_degf };
    Rotatorf loaded = RoundTrip(original);
    EXPECT_FLOAT_EQ(*loaded.pitch, *original.pitch);
    EXPECT_FLOAT_EQ(*loaded.yaw, *original.yaw);
    EXPECT_FLOAT_EQ(*loaded.roll, *original.roll);
}

// --- Math: AABB ---
TEST_F(AutoSerializeTest, Math_AABB)
{
    AABBf original;
    original.min = { -1.0f, -2.0f, -3.0f };
    original.max = { 4.0f, 5.0f, 6.0f };
    AABBf loaded = RoundTrip(original);
    EXPECT_EQ(loaded.min, original.min);
    EXPECT_EQ(loaded.max, original.max);
}

// --- Math: LinearColor ---
TEST_F(AutoSerializeTest, Math_LinearColor)
{
    LinearColor original{ 0.2f, 0.4f, 0.6f, 0.8f };
    LinearColor loaded = RoundTrip(original);
    EXPECT_FLOAT_EQ(loaded.r, original.r);
    EXPECT_FLOAT_EQ(loaded.g, original.g);
    EXPECT_FLOAT_EQ(loaded.b, original.b);
    EXPECT_FLOAT_EQ(loaded.a, original.a);
}

// --- Math: Color (uint8) ---
TEST_F(AutoSerializeTest, Math_Color)
{
    Color original{ 255, 128, 64, 32 };
    Color loaded = RoundTrip(original);
    EXPECT_EQ(loaded.r, original.r);
    EXPECT_EQ(loaded.g, original.g);
    EXPECT_EQ(loaded.b, original.b);
    EXPECT_EQ(loaded.a, original.a);
}

// --- FixedArray ---
TEST_F(AutoSerializeTest, FixedArray_RoundTrip)
{
    FixedArray<int32, 4> original;
    original[0] = 10;
    original[1] = 20;
    original[2] = 30;
    original[3] = 40;

    FixedArray<int32, 4> loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
}

// --- Enum ---
TEST_F(AutoSerializeTest, Enum_RoundTrip)
{
    ETestColor original = ETestColor::Blue;
    ETestColor loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
}

// --- Guid 단독 ---
TEST_F(AutoSerializeTest, Guid_RoundTrip)
{
    Guid original = Guid::NewGuid();
    Guid loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
    EXPECT_TRUE(loaded.IsValid());
}

// --- StringName 단독 ---
TEST_F(AutoSerializeTest, StringName_RoundTrip)
{
    StringName original("TestStringName");
    StringName loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
}

// --- TypeId 단독 ---
TEST_F(AutoSerializeTest, TypeId_RoundTrip)
{
    TypeId original = TypeId::Get<SimpleData>();
    TypeId loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
}

// --- MeshPrimitives: Vertex ---
TEST_F(AutoSerializeTest, Vertex_RoundTrip)
{
    StaticVertex original;
    original.position = { 1.0f, 2.0f, 3.0f };
    original.normal = { 0.0f, 1.0f, 0.0f };
    original.tex_coord = { 0.5f, 0.75f };
    original.tangent = { 1.0f, 0.0f, 0.0f, 1.0f };

    StaticVertex loaded = RoundTrip(original);
    EXPECT_EQ(loaded.position, original.position);
    EXPECT_EQ(loaded.normal, original.normal);
    EXPECT_EQ(loaded.tex_coord, original.tex_coord);
    EXPECT_EQ(loaded.tangent, original.tangent);
}

// --- MeshPrimitives: SkinVertex ---
TEST_F(AutoSerializeTest, SkinVertex_RoundTrip)
{
    SkinVertex original;
    original.bone_indices[0] = 0;
    original.bone_indices[1] = 1;
    original.bone_indices[2] = 2;
    original.bone_indices[3] = 3;
    original.bone_weights[0] = 0.5f;
    original.bone_weights[1] = 0.3f;
    original.bone_weights[2] = 0.15f;
    original.bone_weights[3] = 0.05f;

    SkinVertex loaded = RoundTrip(original);
    EXPECT_EQ(loaded.bone_indices, original.bone_indices);
    EXPECT_EQ(loaded.bone_weights, original.bone_weights);
}

// --- Enum: SE_REFLECT_ENUM으로 등록된 enum -> TypeInfo 경유 직렬화 ---
TEST_F(AutoSerializeTest, ReflectedEnum_ViaTypeInfo)
{
    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<ETestColor>());
    ASSERT_NE(info.serialize, nullptr);
    EXPECT_EQ(info.kind, ETypeKind::Enum);

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    ETestColor original = ETestColor::Green;
    info.serialize(writer, &original);

    MemoryReader reader(buffer);
    ETestColor loaded = ETestColor::Red;
    info.serialize(reader, &loaded);
    EXPECT_EQ(loaded, ETestColor::Green);
}

// ============================================================================
//  일반적이지 않은 TC (Edge / Unusual Cases)
// ============================================================================

// --- Transient 프로퍼티는 직렬화에서 제외 ---
TEST_F(AutoSerializeTest, TransientProperty_Skipped)
{
    TransientData original{ .saved_val = 42, .transient_val = 999 };

    // AutoSerialize로 직렬화 -> saved_val만 저장, transient_val은 건너뜀
    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<TransientData>());

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    TransientData copy = original;
    info.serialize(writer, &copy);

    // 로드 시 transient_val은 기본값(0)을 유지해야 함
    MemoryReader reader(buffer);
    TransientData loaded;
    loaded.transient_val = 777;  // 초기값을 다른 값으로 설정
    info.serialize(reader, &loaded);

    EXPECT_EQ(loaded.saved_val, 42);       // 직렬화된 값
    EXPECT_EQ(loaded.transient_val, 777);  // 변경되지 않음 (Transient)
}

// --- 빈 구조체 AutoSerialize ---
TEST_F(AutoSerializeTest, EmptyStruct)
{
    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<EmptyReflected>());
    ASSERT_NE(info.serialize, nullptr);

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    EmptyReflected original;
    info.serialize(writer, &original);

    // 빈 구조체이므로 데이터가 거의 없어야 함
    MemoryReader reader(buffer);
    EmptyReflected loaded;
    info.serialize(reader, &loaded);
    EXPECT_EQ(loaded, original);
}

// --- 무효한 AssetId (기본 생성) ---
TEST_F(AutoSerializeTest, AssetId_Invalid)
{
    AssetId original;  // Invalid
    EXPECT_FALSE(original.IsValid());

    AssetId loaded = RoundTrip(original);
    EXPECT_FALSE(loaded.IsValid());
    EXPECT_EQ(loaded, original);
    EXPECT_EQ(loaded, AssetId::Invalid);
}

// --- 무효한 Entity (기본 생성) ---
TEST_F(AutoSerializeTest, Entity_Invalid)
{
    Entity original;  // Invalid, generation=0
    EXPECT_FALSE(original.IsValid());

    Entity loaded = RoundTrip(original);
    EXPECT_FALSE(loaded.IsValid());
    EXPECT_EQ(loaded, original);
}

// --- NaN float 직렬화 ---
TEST_F(AutoSerializeTest, Float_NaN)
{
    float original = std::numeric_limits<float>::quiet_NaN();

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    writer << original;

    MemoryReader reader(buffer);
    float loaded = 0.0f;
    reader << loaded;

    EXPECT_TRUE(std::isnan(loaded));
}

// --- Infinity float 직렬화 ---
TEST_F(AutoSerializeTest, Float_Infinity)
{
    float pos_inf = std::numeric_limits<float>::infinity();
    float neg_inf = -std::numeric_limits<float>::infinity();

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    writer << pos_inf << neg_inf;

    MemoryReader reader(buffer);
    float loaded_pos = 0.0f;
    float loaded_neg = 0.0f;
    reader << loaded_pos << loaded_neg;

    EXPECT_TRUE(std::isinf(loaded_pos));
    EXPECT_GT(loaded_pos, 0.0f);
    EXPECT_TRUE(std::isinf(loaded_neg));
    EXPECT_LT(loaded_neg, 0.0f);
}

// --- NaN이 포함된 Vector3 ---
TEST_F(AutoSerializeTest, Math_Vector3_NaN)
{
    Vector3f original{ std::numeric_limits<float>::quiet_NaN(), 0.0f, 1.0f };

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    writer << original;

    MemoryReader reader(buffer);
    Vector3f loaded{};
    reader << loaded;

    EXPECT_TRUE(std::isnan(loaded.x));
    EXPECT_FLOAT_EQ(loaded.y, 0.0f);
    EXPECT_FLOAT_EQ(loaded.z, 1.0f);
}

// --- Zero Vector ---
TEST_F(AutoSerializeTest, Math_ZeroVector)
{
    Vector3f original{ 0.0f, 0.0f, 0.0f };
    Vector3f loaded = RoundTrip(original);
    EXPECT_FLOAT_EQ(loaded.x, 0.0f);
    EXPECT_FLOAT_EQ(loaded.y, 0.0f);
    EXPECT_FLOAT_EQ(loaded.z, 0.0f);
}

// --- Identity Quaternion ---
TEST_F(AutoSerializeTest, Math_IdentityQuaternion)
{
    Quaternionf original{ 0.0f, 0.0f, 0.0f, 1.0f };
    Quaternionf loaded = RoundTrip(original);
    EXPECT_FLOAT_EQ(loaded.x, 0.0f);
    EXPECT_FLOAT_EQ(loaded.y, 0.0f);
    EXPECT_FLOAT_EQ(loaded.z, 0.0f);
    EXPECT_FLOAT_EQ(loaded.w, 1.0f);
}

// --- FixedArray: 모든 원소 동일 ---
TEST_F(AutoSerializeTest, FixedArray_AllSame)
{
    FixedArray<float, 4> original;
    original.Fill(42.0f);

    FixedArray<float, 4> loaded = RoundTrip(original);
    EXPECT_EQ(loaded, original);
    for (usize i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ(loaded[i], 42.0f);
    }
}

// --- Enum: 최대 underlying 값 ---
TEST_F(AutoSerializeTest, Enum_MaxUnderlying)
{
    ETestColor original = ETestColor::Alpha;  // 255 (uint8 max)
    ETestColor loaded = RoundTrip(original);
    EXPECT_EQ(loaded, ETestColor::Alpha);
}

// --- 기존 값을 덮어쓰는 로드 ---
TEST_F(AutoSerializeTest, OverwriteExistingValues)
{
    SimpleData original{ .x = 100, .y = 99.9f, .name = "Overwrite" };

    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<SimpleData>());

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    SimpleData copy = original;
    info.serialize(writer, &copy);

    // 기존에 다른 값이 들어있는 인스턴스
    SimpleData loaded{ .x = -1, .y = -1.0f, .name = "OldValue" };

    MemoryReader reader(buffer);
    info.serialize(reader, &loaded);

    EXPECT_EQ(loaded, original);
    EXPECT_EQ(loaded.x, 100);
    EXPECT_FLOAT_EQ(loaded.y, 99.9f);
    EXPECT_EQ(loaded.name, "Overwrite");
}

// --- 중첩 ADL 타입 컨테이너: Array<Entity> ---
TEST_F(AutoSerializeTest, ArrayOfEntities)
{
    // binary로 Entity 3개를 만들어서 Array에 담기
    Array<uint8> entity_buf;
    {
        MemoryWriter w(entity_buf);
        // Entity 3개의 id, generation 쌍
        uint32 ids[] = { 0, 1, 2 };
        uint32 gens[] = { 1, 1, 1 };
        for (int i = 0; i < 3; ++i)
        {
            w << ids[i] << gens[i];
        }
    }

    // Entity 3개를 로드
    Array<Entity> entities;
    {
        MemoryReader r(entity_buf);
        for (int i = 0; i < 3; ++i)
        {
            Entity e;
            r << e;
            entities.Push(e);
        }
    }

    // Array<Entity> 라운드트립
    Array<Entity> loaded = RoundTrip(entities);
    EXPECT_EQ(loaded.Len(), 3);
    for (usize i = 0; i < 3; ++i)
    {
        EXPECT_EQ(loaded[i], entities[i]);
    }
}

// --- 중첩 ADL 타입 Map: HashMap<String, AssetId> ---
TEST_F(AutoSerializeTest, MapOfAssetIds)
{
    HashMap<String, AssetId> original;
    original.Insert("texture", AssetId(Guid::NewGuid()));
    original.Insert("mesh", AssetId(Guid::NewGuid()));
    original.Insert("material", AssetId(Guid::NewGuid()));

    HashMap<String, AssetId> loaded = RoundTrip(original);

    EXPECT_EQ(loaded.Len(), 3);
    EXPECT_EQ(loaded["texture"], original["texture"]);
    EXPECT_EQ(loaded["mesh"], original["mesh"]);
    EXPECT_EQ(loaded["material"], original["material"]);
}

// --- 여러 AutoSerialize 호출 연속 ---
TEST_F(AutoSerializeTest, MultipleSequentialAutoSerialize)
{
    const TypeInfo& simple_info = TypeRegistry::Get().FindChecked(TypeId::Get<SimpleData>());
    const TypeInfo& container_info = TypeRegistry::Get().FindChecked(TypeId::Get<ContainerData>());

    SimpleData s1{ .x = 1, .y = 1.0f, .name = "First" };
    ContainerData c1;
    c1.numbers = { 10, 20 };
    c1.scores.Insert("test", 100.0f);
    SimpleData s2{ .x = 2, .y = 2.0f, .name = "Second" };

    Array<uint8> buffer;
    MemoryWriter writer(buffer);
    simple_info.serialize(writer, &s1);
    container_info.serialize(writer, &c1);
    simple_info.serialize(writer, &s2);

    MemoryReader reader(buffer);
    SimpleData rs1;
    SimpleData rs2;
    ContainerData rc1;
    simple_info.serialize(reader, &rs1);
    container_info.serialize(reader, &rc1);
    simple_info.serialize(reader, &rs2);

    EXPECT_EQ(rs1, s1);
    EXPECT_EQ(rc1, c1);
    EXPECT_EQ(rs2, s2);
}

// --- TypeInfo 등록 상태 검증 ---
TEST_F(AutoSerializeTest, TypeInfo_CorrectKind)
{
    // Struct
    {
        const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<SimpleData>());
        EXPECT_EQ(info.kind, ETypeKind::Struct);
        EXPECT_NE(info.serialize, nullptr);
    }
    // Enum
    {
        const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<ETestColor>());
        EXPECT_EQ(info.kind, ETypeKind::Enum);
        EXPECT_NE(info.serialize, nullptr);
    }
}

// --- DerivedData의 base_or_inner_id가 BaseData를 가리키는지 검증 ---
TEST_F(AutoSerializeTest, Inheritance_BaseIdSet)
{
    const TypeInfo& info = TypeRegistry::Get().FindChecked(TypeId::Get<DerivedData>());
    EXPECT_EQ(info.base_or_inner_id, TypeId::Get<BaseData>());
}

// --- Ray 라운드트립 ---
TEST_F(AutoSerializeTest, Math_Ray)
{
    Rayf original;
    original.origin = { 0.0f, 1.0f, 2.0f };
    original.direction = { 0.0f, 0.0f, 1.0f };

    Rayf loaded = RoundTrip(original);
    EXPECT_EQ(loaded.origin, original.origin);
    EXPECT_EQ(loaded.direction, original.direction);
}

// --- Vector2, Vector4 라운드트립 ---
TEST_F(AutoSerializeTest, Math_Vector2_And_Vector4)
{
    Vector2f v2{ 1.5f, -2.5f };
    Vector2f v2_loaded = RoundTrip(v2);
    EXPECT_FLOAT_EQ(v2_loaded.x, v2.x);
    EXPECT_FLOAT_EQ(v2_loaded.y, v2.y);

    Vector4f v4{ 1.0f, 2.0f, 3.0f, 4.0f };
    Vector4f v4_loaded = RoundTrip(v4);
    EXPECT_FLOAT_EQ(v4_loaded.x, v4.x);
    EXPECT_FLOAT_EQ(v4_loaded.y, v4.y);
    EXPECT_FLOAT_EQ(v4_loaded.z, v4.z);
    EXPECT_FLOAT_EQ(v4_loaded.w, v4.w);
}

// --- 매우 큰 경계값의 Guid ---
TEST_F(AutoSerializeTest, Guid_None)
{
    Guid original = Guid::None;
    Guid loaded = RoundTrip(original);
    EXPECT_EQ(loaded, Guid::None);
    EXPECT_FALSE(loaded.IsValid());
}
