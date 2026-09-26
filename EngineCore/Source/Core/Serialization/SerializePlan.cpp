#include "SimpleEngine/Core/Serialization/SerializePlan.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashSet.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"
#include "SimpleEngine/Utility/HashUtils.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include <concepts>
#include <type_traits>
#include <utility>


namespace se
{
namespace
{
/** TypeId마다 Plan 하나를 보관하는 저장소를 가져옵니다. */
HashMap<TypeId, SerializePlan>& PlanStorage()
{
    static HashMap<TypeId, SerializePlan> storage;
    return storage;
}

/** StructSteps::fields가 가리키는 FieldStep 배열의 저장소를 가져옵니다. */
HashMap<TypeId, Array<FieldStep>>& FieldStepStorage()
{
    static HashMap<TypeId, Array<FieldStep>> storage;
    return storage;
}

/** 오류 메시지에 쓸 타입 이름을 돌려줍니다. TypeRegistry에 없는 타입이면 "type id <값>"을 돌려줍니다. */
[[nodiscard]] String DisplayNameOf(TypeId id)
{
    if (const auto info = TypeRegistry::Get().Find(id))
    {
        return info->name;
    }
    return String::Format("type id {}", id.Value());
}

/** 산술 타입 T의 값을 Bool/Int/Float 노드로 쓰고 읽는 SerializeOps를 만듭니다. */
template <typename T>
SerializeOps MakePrimitiveOps()
{
    SerializeOps ops{};

    if constexpr (std::same_as<T, bool>)
    {
        ops.write = [](ArchiveWriter& writer, const void* value) static
        {
            writer.Bool(*static_cast<const bool*>(value));
        };
        ops.read = [](ArchiveReader& reader, void* value) static
        {
            bool temp = false;
            reader.Bool(temp);
            if (!reader.HasError())
            {
                *static_cast<bool*>(value) = temp;
            }
        };
    }
    else if constexpr (std::floating_point<T>)
    {
        ops.write = [](ArchiveWriter& writer, const void* value) static
        {
            writer.Float(static_cast<f64>(*static_cast<const T*>(value)), serde::FloatWidthOf<T>());
        };
        ops.read = [](ArchiveReader& reader, void* value) static
        {
            f64 temp = 0.0;
            reader.Float(temp, serde::FloatWidthOf<T>());
            if (!reader.HasError())
            {
                *static_cast<T*>(value) = static_cast<T>(temp);
            }
        };
    }
    else
    {
        ops.write = [](ArchiveWriter& writer, const void* value) static
        {
            writer.Int(static_cast<i64>(*static_cast<const T*>(value)), serde::IntWidthOf<T>(), std::is_signed_v<T>);
        };
        ops.read = [](ArchiveReader& reader, void* value) static
        {
            i64 temp = 0;
            reader.Int(temp, serde::IntWidthOf<T>(), std::is_signed_v<T>);
            if (!reader.HasError())
            {
                *static_cast<T*>(value) = static_cast<T>(temp);
            }
        };
    }

    return ops;
}

/** 정수 타입을 Int 노드로 쓸 때의 폭과 부호 */
struct IntFormat
{
    EIntWidth width = EIntWidth::Bits8;
    bool is_signed = false;
};

/** 허용된 산술 타입 하나의 정보 */
struct PrimitiveEntry
{
    TypeId id;
    SerializeOps ops{};
    Optional<IntFormat> int_format;
};

/** 산술 타입 T의 PrimitiveEntry를 만듭니다. bool을 뺀 정수와 문자 타입이면 int_format도 채웁니다. */
template <typename T>
PrimitiveEntry MakePrimitiveEntry()
{
    PrimitiveEntry entry{ .id = TypeId::Of<T>(), .ops = MakePrimitiveOps<T>() };
    if constexpr (std::integral<T> && !std::same_as<T, bool>)
    {
        entry.int_format = IntFormat{ .width = serde::IntWidthOf<T>(), .is_signed = std::is_signed_v<T> };
    }
    return entry;
}

/** id가 직렬화할 수 있는 산술 타입(bool, 문자, 정수, 실수)이면 그 PrimitiveEntry를, 아니면 NullOpt를 돌려줍니다. */
[[nodiscard]] Optional<const PrimitiveEntry&> FindPrimitive(TypeId id)
{
    static const PrimitiveEntry entries[] = {
        MakePrimitiveEntry<bool>(),
        MakePrimitiveEntry<char>(),
        MakePrimitiveEntry<char8_t>(),
        MakePrimitiveEntry<char16_t>(),
        MakePrimitiveEntry<char32_t>(),
        MakePrimitiveEntry<i8>(),
        MakePrimitiveEntry<i16>(),
        MakePrimitiveEntry<i32>(),
        MakePrimitiveEntry<i64>(),
        MakePrimitiveEntry<u8>(),
        MakePrimitiveEntry<u16>(),
        MakePrimitiveEntry<u32>(),
        MakePrimitiveEntry<u64>(),
        MakePrimitiveEntry<f32>(),
        MakePrimitiveEntry<f64>(),
    };

    for (const PrimitiveEntry& entry : entries)
    {
        if (entry.id == id)
        {
            return entry;
        }
    }
    return NullOpt;
}

/**
 * 트레이트가 없는 Opaque 타입 id의 leaf ops를 구합니다.
 * 직렬화할 수 있는 산술 타입이면 그 ops를, wchar_t/long double이나 그 밖의 Opaque 타입이면 오류를 돌려줍니다.
 */
[[nodiscard]] Expected<SerializeOps, String> CompilePrimitiveLeaf(TypeId id)
{
    if (id == TypeId::Of<wchar_t>())
    {
        return Unexpected{ "wchar_t has a platform-defined width and cannot be serialized" };
    }
    if (id == TypeId::Of<long double>())
    {
        return Unexpected{ "long double has a platform-defined width and cannot be serialized" };
    }

    if (const auto primitive = FindPrimitive(id))
    {
        return primitive->ops;
    }
    return Unexpected{ "no SerializeTraits registered for this opaque type" };
}

// forward declaration
[[nodiscard]] Expected<const SerializePlan*, String> CompileRecursive(TypeId id, Array<TypeId>& newly_inserted);

/**
 * struct_id 구조체의 필드를 베이스 필드부터 선언 순서대로 flat_fields에 추가하고, 필드마다 그 타입의 Plan을 연결합니다.
 * 오프셋은 base_offset을 더한 최상위 구조체 기준입니다. 베이스에 트레이트가 있거나, 베이스가 Struct가 아니거나, 필드 이름이 겹치면 오류입니다.
 */
[[nodiscard]] Expected<void, String> FlattenFields(TypeId struct_id, usize base_offset, Array<FieldStep>& flat_fields, Array<TypeId>& newly_inserted) // NOLINT(*-no-recursion)
{
    const StructInfo& struct_info = TypeRegistry::Get().FindChecked(struct_id).AsStruct().Value();

    // 베이스 자신의 Plan은 만들지 않고 필드만 펼쳐 넣음 (부모 필드가 먼저)
    for (const BaseInfo& base : struct_info.bases)
    {
        // 필드를 펼치면 베이스의 트레이트를 쓸 수 없으므로, 조용히 무시하지 않고 오류로 처리
        if (SerializeOpsRegistry::Get().Find(base.type).HasValue())
        {
            return Unexpected{
                String::Format("base '{}' has a registered SerializeTraits; flattening would bypass it", DisplayNameOf(base.type))
            };
        }

        const auto base_info = TypeRegistry::Get().Find(base.type);
        if (!base_info.HasValue() || !base_info->AsStruct().HasValue())
        {
            return Unexpected{ String::Format("base '{}' is not a struct", DisplayNameOf(base.type)) };
        }

        if (auto result = FlattenFields(base.type, base_offset + base.offset, flat_fields, newly_inserted); result.HasError())
        {
            return result;
        }
    }

    for (const FieldInfo& field : struct_info.fields)
    {
        // 섀도잉이나 비가상 다이아몬드로 같은 이름이 두 번 나오면 텍스트의 키가 겹치므로 오류로 판단
        for (const FieldStep& existing : flat_fields)
        {
            if (existing.name == field.name)
            {
                return Unexpected{ String::Format("duplicate field name '{}' after base flattening", field.name) };
            }
        }

        // 필드 타입의 Plan을 새로 만들거나, 이미 있는 것(만드는 중인 슬롯 포함)의 주소를 가져옴
        const auto field_plan = CompileRecursive(field.type, newly_inserted);
        if (field_plan.HasError())
        {
            return Unexpected{
                String::Format("field '{}' of type '{}': {}", field.name, DisplayNameOf(field.type), field_plan.Error())
            };
        }

        flat_fields.Push({
            .name = field.name,
            .offset = base_offset + field.offset,
            .plan = *field_plan,
        });
    }

    return {};
}

/** 구조체 id의 StructSteps를 만듭니다. */
[[nodiscard]] Expected<PlanSteps, String> CompileStruct(TypeId id, Array<TypeId>& newly_inserted)
{
    Array<FieldStep> flat_fields;
    if (auto result = FlattenFields(id, 0, flat_fields, newly_inserted); result.HasError())
    {
        return Unexpected{ std::move(result).Error() };
    }

    Array<FieldStep>& stored = FieldStepStorage().Emplace(id, std::move(flat_fields));
    return PlanSteps{ StructSteps{ .fields = ArrayView<const FieldStep>(stored) } };
}

/** Array-like 컨테이너 id의 ArraySteps를 만듭니다. */
[[nodiscard]] Expected<PlanSteps, String> CompileArray(TypeId id, const ArrayInfo& a, Array<TypeId>& newly_inserted)
{
    auto element_plan = CompileRecursive(a.element, newly_inserted);
    if (element_plan.HasError())
    {
        return Unexpected{ std::move(element_plan).Error() };
    }

    const ValueOps& container_ops = ValueOpsRegistry::Get().Find(id).Value();
    const ArrayOps& array_ops = container_ops.AsArray().Value();
    return PlanSteps{ ArraySteps{ .element = element_plan.Value(), .ops = &array_ops } };
}

/** Set-like 컨테이너 id의 SetSteps를 만듭니다. */
[[nodiscard]] Expected<PlanSteps, String> CompileSet(TypeId id, const SetInfo& s, Array<TypeId>& newly_inserted)
{
    auto element_plan = CompileRecursive(s.element, newly_inserted);
    if (element_plan.HasError())
    {
        return Unexpected{ std::move(element_plan).Error() };
    }

    const ValueOps& container_ops = ValueOpsRegistry::Get().Find(id).Value();
    const SetOps& set_ops = container_ops.AsSet().Value();

    const TypeInfo& element_info = TypeRegistry::Get().FindChecked(s.element);
    const ValueOps& element_value_ops = ValueOpsRegistry::Get().Find(s.element).Value();

    return PlanSteps{
        SetSteps{
            .element = element_plan.Value(),
            .ops = &set_ops,
            .element_info = {
                .size = element_info.size,
                .alignment = element_info.alignment,
                .value_ops = &element_value_ops,
            },
        }
    };
}

/** Map-like 컨테이너 id의 MapSteps를 만듭니다. */
[[nodiscard]] Expected<PlanSteps, String> CompileMap(TypeId id, const MapInfo& m, Array<TypeId>& newly_inserted)
{
    auto key_plan = CompileRecursive(m.key, newly_inserted);
    if (key_plan.HasError())
    {
        return Unexpected{ std::move(key_plan).Error() };
    }
    auto value_plan = CompileRecursive(m.value, newly_inserted);
    if (value_plan.HasError())
    {
        return Unexpected{ std::move(value_plan).Error() };
    }

    const ValueOps& container_ops = ValueOpsRegistry::Get().Find(id).Value();
    const MapOps& map_ops = container_ops.AsMap().Value();

    const TypeInfo& key_info = TypeRegistry::Get().FindChecked(m.key);
    const ValueOps& key_value_ops = ValueOpsRegistry::Get().Find(m.key).Value();
    const TypeInfo& value_info = TypeRegistry::Get().FindChecked(m.value);
    const ValueOps& value_value_ops = ValueOpsRegistry::Get().Find(m.value).Value();

    return PlanSteps{
        MapSteps{
            .key = key_plan.Value(),
            .value = value_plan.Value(),
            .ops = &map_ops,
            .key_info = {
                .size = key_info.size,
                .alignment = key_info.alignment,
                .value_ops = &key_value_ops,
            },
            .value_info = {
                .size = value_info.size,
                .alignment = value_info.alignment,
                .value_ops = &value_value_ops,
            },
        }
    };
}

/** Optional 타입 id의 OptionalSteps를 만듭니다. */
[[nodiscard]] Expected<PlanSteps, String> CompileOptional(TypeId id, const OptionalInfo& o, Array<TypeId>& newly_inserted)
{
    auto inner_plan = CompileRecursive(o.inner, newly_inserted);
    if (inner_plan.HasError())
    {
        return Unexpected{ std::move(inner_plan).Error() };
    }

    const ValueOps& container_ops = ValueOpsRegistry::Get().Find(id).Value();
    const OptionalOps& optional_ops = container_ops.AsOptional().Value();
    return PlanSteps{ OptionalSteps{ .inner = inner_plan.Value(), .ops = &optional_ops } };
}

/** enum의 EnumStep을 만듭니다.*/
[[nodiscard]] Expected<PlanSteps, String> CompileEnum(const EnumInfo& e)
{
    if (e.underlying == TypeId::Of<wchar_t>())
    {
        return Unexpected{ "enum underlying type wchar_t has a platform-defined width and cannot be serialized" };
    }

    const auto primitive = FindPrimitive(e.underlying);
    if (!primitive.HasValue() || !primitive->int_format.HasValue())
    {
        return Unexpected{ String::Format("enum underlying type '{}' is not an integer type", DisplayNameOf(e.underlying)) };
    }

    const IntFormat& format = *primitive->int_format;
    return PlanSteps{ EnumStep{ .width = format.width, .is_signed = format.is_signed, .entries = e.entries } };
}

/**
 * id의 Plan을 찾거나 새로 만들어 주소를 돌려줍니다. 필드와 원소 같은 자식 타입의 Plan도 함께 만듭니다.
 * 새로 넣은 Plan 슬롯의 TypeId는 실패했을 때 지울 수 있도록 newly_inserted에 기록합니다.
 */
[[nodiscard]] Expected<const SerializePlan*, String> CompileRecursive(TypeId id, Array<TypeId>& newly_inserted)
{
    // 이미 Storage가 만들어져 있다면 그 주소를 반환
    if (const auto cached = PlanStorage().Find(id))
    {
        return &cached.Value();
    }

    // SerializeOpsRegistry에 등록되어있다는건, 현재 타입이 Leaf 라는 것. (Leaf가 직접적인 직렬화를 수행하기 때문)
    if (const auto ops = SerializeOpsRegistry::Get().Find(id))
    {
        SerializePlan& slot = PlanStorage().Emplace(id, SerializePlan{ .type = id, .steps = PlanSteps{ LeafStep{ .ops = *ops } } });
        newly_inserted.Push(id);
        return &slot;
    }

    // TypeRegistry에 없는 타입은 return
    const auto maybe_info = TypeRegistry::Get().Find(id);
    if (!maybe_info.HasValue())
    {
        return Unexpected{ "type is not registered" };
    }
    const TypeInfo& info = maybe_info.Value();

    // 빈 슬롯을 먼저 생성. 추가로 실패했을 때 TryOf가 지울 수 있게 newly_inserted에 기록
    SerializePlan& slot = PlanStorage().Emplace(id, SerializePlan{ .type = id, .steps = PlanSteps{ LeafStep{} } });
    newly_inserted.Push(id);

    // 형태별 steps를 생성
    auto steps_result = info.VisitShape(
        [&](const OpaqueInfo&) -> Expected<PlanSteps, String>
        {
            auto leaf = CompilePrimitiveLeaf(id);
            if (leaf.HasError())
            {
                return Unexpected{ std::move(leaf).Error() };
            }
            return PlanSteps{ LeafStep{ .ops = std::move(leaf).Value() } };
        },
        [&](const StructInfo&) { return CompileStruct(id, newly_inserted); },
        [&](const ArrayInfo& a) { return CompileArray(id, a, newly_inserted); },
        [&](const SetInfo& s) { return CompileSet(id, s, newly_inserted); },
        [&](const MapInfo& m) { return CompileMap(id, m, newly_inserted); },
        [&](const OptionalInfo& o) { return CompileOptional(id, o, newly_inserted); },
        [&](const EnumInfo& e) { return CompileEnum(e); }
    );

    if (steps_result.HasError())
    {
        return Unexpected{ std::move(steps_result).Error() };
    }

    // 자식 Plan이 모두 준비되면 슬롯을 설정
    slot.steps = std::move(steps_result).Value();
    return &slot;
}

/** 스키마 서술에서 형태를 구분하는 태그. */
enum class ESchemaNode : u8
{
    Leaf = 1,
    Struct = 2,
    Array = 3,
    Set = 4,
    Map = 5,
    Optional = 6,
    Enum = 7,
};

/** 스키마 서술을 FNV-1a로 누적합니다. */
class SchemaHasher
{
public:
    void U64(u64 value)
    {
        detail::FNV1a_U64(hash, value);
    }

    /** 길이를 먼저 넣어 이어진 두 문자열의 경계가 해시에 남게 합니다. */
    void Str(StringView value)
    {
        U64(value.ByteLen());
        hash = hash::FNV(value, hash);
    }

    void Node(ESchemaNode node)
    {
        U64(static_cast<u64>(node));
    }

    [[nodiscard]] u64 Get() const { return hash; }

private:
    u64 hash = detail::FNV_OFFSET_BASIS;
};

/** plan이 직접 가리키는 자식 Plan마다 func를 호출합니다. */
template <typename Fn>
void ForEachChildPlan(const SerializePlan& plan, Fn&& func)
{
    std::visit(Overloaded{
        [](const LeafStep&) {},
        [&](const StructSteps& steps)
        {
            for (const FieldStep& field : steps.fields)
            {
                func(field.plan);
            }
        },
        [&](const ArraySteps& steps) { func(steps.element); },
        [&](const SetSteps& steps) { func(steps.element); },
        [&](const MapSteps& steps)
        {
            func(steps.key);
            func(steps.value);
        },
        [&](const OptionalSteps& steps) { func(steps.inner); },
        [](const EnumStep&) {},
    }, plan.steps);
}

/** root에서 닿는 Plan을 모두 모아 TypeId 순으로 정렬해 돌려줍니다. */
[[nodiscard]] Array<const SerializePlan*> CollectReachablePlans(const SerializePlan& root)
{
    Array<const SerializePlan*> reachable;
    HashSet<TypeId> visited;
    Stack<const SerializePlan*> pending;

    visited.Insert(root.type);
    pending.Push(&root);
    while (const auto plan = pending.Pop())
    {
        reachable.Push(*plan);
        ForEachChildPlan(**plan, [&](const SerializePlan* child)
        {
            if (visited.Insert(child->type))
            {
                pending.Push(child);
            }
        });
    }

    reachable.Sort([](const SerializePlan* lhs, const SerializePlan* rhs) { return lhs->type.Value() < rhs->type.Value(); });
    return reachable;
}

/** plan 자신의 서술을 넣습니다. 자식은 해시가 아니라 TypeId로만 참조합니다. */
void DescribePlan(SchemaHasher& hasher, const SerializePlan& plan)
{
    hasher.U64(plan.type.Value());
    std::visit(Overloaded{
        [&](const LeafStep& leaf)
        {
            // 산술 타입의 폭, 부호, 종류는 TypeId가 정하므로 트레이트 버전만 더 넣음
            hasher.Node(ESchemaNode::Leaf);
            hasher.U64(leaf.ops.format_version);
        },
        [&](const StructSteps& steps)
        {
            hasher.Node(ESchemaNode::Struct);
            hasher.U64(steps.fields.Len());
            for (const FieldStep& field : steps.fields)
            {
                hasher.Str(field.name);
                hasher.U64(field.plan->type.Value());
            }
        },
        [&](const ArraySteps& steps)
        {
            hasher.Node(ESchemaNode::Array);
            hasher.U64(steps.element->type.Value());
        },
        [&](const SetSteps& steps)
        {
            hasher.Node(ESchemaNode::Set);
            hasher.U64(steps.element->type.Value());
        },
        [&](const MapSteps& steps)
        {
            hasher.Node(ESchemaNode::Map);
            hasher.U64(steps.key->type.Value());
            hasher.U64(steps.value->type.Value());
        },
        [&](const OptionalSteps& steps)
        {
            hasher.Node(ESchemaNode::Optional);
            hasher.U64(steps.inner->type.Value());
        },
        [&](const EnumStep& e)
        {
            hasher.Node(ESchemaNode::Enum);
            hasher.U64(static_cast<u64>(e.width));
            hasher.U64(static_cast<u64>(e.is_signed));
            hasher.U64(e.entries.Len());
            for (const EnumEntry& entry : e.entries)
            {
                hasher.U64(static_cast<u64>(entry.value));
                hasher.Str(entry.name);
            }
        },
    }, plan.steps);
}
} // namespace

Expected<const SerializePlan*, String> SerializePlan::TryOf(TypeId id)
{
    Array<TypeId> newly_inserted;
    auto result = CompileRecursive(id, newly_inserted);
    if (result.HasError())
    {
        HashMap<TypeId, SerializePlan>& plans = PlanStorage();
        HashMap<TypeId, Array<FieldStep>>& fields = FieldStepStorage();
        for (const TypeId inserted_id : newly_inserted)
        {
            plans.Remove(inserted_id);
            fields.Remove(inserted_id);
        }

        return Unexpected{ String::Format("SerializePlan: cannot compile '{}': {}", DisplayNameOf(id), result.Error()) };
    }

    return result;
}

u64 SerializePlan::SchemaHash() const
{
    SchemaHasher hasher;
    for (const SerializePlan* plan : CollectReachablePlans(*this))
    {
        DescribePlan(hasher, *plan);
    }
    return hasher.Get();
}
} // namespace se
