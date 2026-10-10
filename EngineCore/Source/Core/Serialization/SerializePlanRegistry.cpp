#include "SimpleEngine/Core/Serialization/SerializePlanRegistry.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"
#include "SimpleEngine/Core/Serialization/Transient.h"

#include <concepts>
#include <mutex>
#include <type_traits>
#include <utility>


namespace se
{
namespace
{
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
 * steps까지 만든 타입의 Binary 인코딩이 메모리 바이트와 똑같은지(trivially packable) 판단합니다.
 * bool을 뺀 산술 타입과, trivially packable한 필드만 오프셋 순서로 빈틈없이 등록해 합이 sizeof와 같은 구조체가 해당합니다.
 */
[[nodiscard]] bool IsTriviallyPackable(const TypeInfo& info, const PlanSteps& steps)
{
    // 여기까지 온 Opaque 타입은 트레이트 없는 산술 타입. bool은 읽을 때 0/1로 바꾸므로 제외
    if (info.IsOpaque())
    {
        return info.id != TypeId::Of<bool>();
    }

    const StructSteps* const struct_steps = std::get_if<StructSteps>(&steps);
    if (struct_steps == nullptr)
    {
        return false;
    }

    // 베이스까지 펼친 필드가 0부터 빈틈없이 이어져야 함. vptr, 패딩, 등록하지 않은 멤버가 있으면 실패
    usize next_offset = 0;
    for (const FieldStep& field : struct_steps->fields)
    {
        if (!field.plan->is_trivially_packable || field.offset != next_offset)
        {
            return false;
        }
        next_offset += TypeRegistry::Get().FindChecked(field.plan->type).size;
    }
    return next_offset == info.size;
}
} // namespace

/**
 * SerializePlanRegistry의 슬롯에 Plan을 컴파일해 넣습니다.
 * 이번 컴파일에서 새로 넣은 슬롯을 기록해 두어, 실패하면 Rollback으로 모두 지울 수 있습니다.
 */
class SerializePlanCompiler
{
public:
    explicit SerializePlanCompiler(SerializePlanRegistry& registry)
        : slots(registry.slots)
    {
    }

    /** id의 Plan을 찾거나 새로 만들어 주소를 돌려줍니다. 필드와 원소 같은 자식 타입의 Plan도 함께 만듭니다. */
    [[nodiscard]] Expected<const SerializePlan*, String> Compile(TypeId id);

    /** 이번 컴파일에서 새로 넣은 슬롯을 모두 지웁니다. */
    void Rollback();

private:
    using Slot = SerializePlanRegistry::Slot;

    [[nodiscard]] Expected<void, String> FlattenFields(TypeId struct_id, usize base_offset, Array<FieldStep>& flat_fields);
    [[nodiscard]] Expected<PlanSteps, String> CompileStruct(TypeId id, Array<FieldStep>& out_fields);
    [[nodiscard]] Expected<PlanSteps, String> CompileArray(TypeId id, const ArrayInfo& a);
    [[nodiscard]] Expected<PlanSteps, String> CompileSet(TypeId id, const SetInfo& s);
    [[nodiscard]] Expected<PlanSteps, String> CompileMap(TypeId id, const MapInfo& m);
    [[nodiscard]] Expected<PlanSteps, String> CompileOptional(TypeId id, const OptionalInfo& o);

private:
    HashMap<TypeId, Slot>& slots;

    /** 이번 컴파일에서 새로 넣은 슬롯의 TypeId */
    Array<TypeId> newly_inserted;
};

/**
 * struct_id 구조체의 필드를 베이스 필드부터 선언 순서대로 flat_fields에 추가하고, 필드마다 그 타입의 Plan을 연결합니다.
 * 오프셋은 base_offset을 더한 최상위 구조체 기준입니다. 베이스에 트레이트가 있거나, 베이스가 Struct가 아니거나, 필드 이름이 겹치면 오류입니다.
 */
Expected<void, String> SerializePlanCompiler::FlattenFields(TypeId struct_id, usize base_offset, Array<FieldStep>& flat_fields) // NOLINT(*-no-recursion)
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

        if (auto result = FlattenFields(base.type, base_offset + base.offset, flat_fields); result.HasError())
        {
            return result;
        }
    }

    for (const FieldInfo& field : struct_info.fields)
    {
        // Transient Field는 Plan에 넣지 않음
        if (field.annotations.Has<serde::TransientAnnotation>())
        {
            continue;
        }

        // 섀도잉이나 비가상 다이아몬드로 같은 이름이 두 번 나오면 텍스트의 키가 겹치므로 오류로 판단
        for (const FieldStep& existing : flat_fields)
        {
            if (existing.name == field.name)
            {
                return Unexpected{ String::Format("duplicate field name '{}' after base flattening", field.name) };
            }
        }

        // 필드 타입의 Plan을 새로 만들거나, 이미 있는 것(만드는 중인 슬롯 포함)의 주소를 가져옴
        const auto field_plan = Compile(field.type);
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

/** 구조체 id의 StructSteps를 만듭니다. 펼친 필드는 그 Plan과 같은 슬롯의 out_fields에 둡니다. */
Expected<PlanSteps, String> SerializePlanCompiler::CompileStruct(TypeId id, Array<FieldStep>& out_fields)
{
    Array<FieldStep> flat_fields;
    if (auto result = FlattenFields(id, 0, flat_fields); result.HasError())
    {
        return Unexpected{ std::move(result).Error() };
    }

    out_fields = std::move(flat_fields);
    return PlanSteps{ StructSteps{ .fields = ArrayView<const FieldStep>(out_fields) } };
}

/** Array-like 컨테이너 id의 ArraySteps를 만듭니다. */
Expected<PlanSteps, String> SerializePlanCompiler::CompileArray(TypeId id, const ArrayInfo& a)
{
    auto element_plan = Compile(a.element);
    if (element_plan.HasError())
    {
        return Unexpected{ std::move(element_plan).Error() };
    }

    const ValueOps& container_ops = ValueOpsRegistry::Get().Find(id).Value();
    const ArrayOps& array_ops = container_ops.AsArray().Value();

    const SerializePlan* const element = element_plan.Value();
    const usize raw_element_size = element->is_trivially_packable && array_ops.element_trivially_copyable
        ? TypeRegistry::Get().FindChecked(a.element).size
        : 0;
    return PlanSteps{ ArraySteps{ .element = element, .ops = &array_ops, .raw_element_size = raw_element_size } };
}

/** Set-like 컨테이너 id의 SetSteps를 만듭니다. */
Expected<PlanSteps, String> SerializePlanCompiler::CompileSet(TypeId id, const SetInfo& s)
{
    auto element_plan = Compile(s.element);
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
Expected<PlanSteps, String> SerializePlanCompiler::CompileMap(TypeId id, const MapInfo& m)
{
    auto key_plan = Compile(m.key);
    if (key_plan.HasError())
    {
        return Unexpected{ std::move(key_plan).Error() };
    }
    auto value_plan = Compile(m.value);
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
Expected<PlanSteps, String> SerializePlanCompiler::CompileOptional(TypeId id, const OptionalInfo& o)
{
    auto inner_plan = Compile(o.inner);
    if (inner_plan.HasError())
    {
        return Unexpected{ std::move(inner_plan).Error() };
    }

    const ValueOps& container_ops = ValueOpsRegistry::Get().Find(id).Value();
    const OptionalOps& optional_ops = container_ops.AsOptional().Value();
    return PlanSteps{ OptionalSteps{ .inner = inner_plan.Value(), .ops = &optional_ops } };
}

Expected<const SerializePlan*, String> SerializePlanCompiler::Compile(TypeId id)
{
    // 이미 슬롯이 있다면 그 Plan의 주소를 반환 (만드는 중인 슬롯 포함)
    if (const auto cached = slots.Find(id))
    {
        return &cached->plan;
    }

    // SerializeOpsRegistry에 등록되어있다는건, 현재 타입이 Leaf 라는 것. (Leaf가 직접적인 직렬화를 수행하기 때문)
    if (const auto ops = SerializeOpsRegistry::Get().Find(id))
    {
        Slot& slot = slots.Emplace(id, Slot{ .plan = SerializePlan{ .type = id, .steps = PlanSteps{ LeafStep{ .ops = *ops } } } });
        newly_inserted.Push(id);
        return &slot.plan;
    }

    // TypeRegistry에 없는 타입은 return
    const auto maybe_info = TypeRegistry::Get().Find(id);
    if (!maybe_info.HasValue())
    {
        return Unexpected{ "type is not registered" };
    }
    const TypeInfo& info = maybe_info.Value();

    // 재귀 참조가 주소를 가져갈 수 있게 빈 슬롯을 먼저 생성. 실패했을 때 Rollback이 지울 수 있게 기록
    Slot& slot = slots.Emplace(id, Slot{ .plan = SerializePlan{ .type = id, .steps = PlanSteps{ PendingStep{} } } });
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
        [&](const StructInfo&) { return CompileStruct(id, slot.fields); },
        [&](const ArrayInfo& a) { return CompileArray(id, a); },
        [&](const SetInfo& s) { return CompileSet(id, s); },
        [&](const MapInfo& m) { return CompileMap(id, m); },
        [&](const OptionalInfo& o) { return CompileOptional(id, o); },
        [&](const EnumInfo& e) { return CompileEnum(e); }
    );

    if (steps_result.HasError())
    {
        return Unexpected{ std::move(steps_result).Error() };
    }

    // 자식 Plan이 모두 준비되면 슬롯을 설정
    slot.plan.steps = std::move(steps_result).Value();
    slot.plan.is_trivially_packable = IsTriviallyPackable(info, slot.plan.steps);
    return &slot.plan;
}

void SerializePlanCompiler::Rollback()
{
    for (const TypeId inserted_id : newly_inserted)
    {
        slots.Remove(inserted_id);
    }
    newly_inserted.Clear();
}


// SerializePlanRegistry
SerializePlanRegistry& SerializePlanRegistry::Get()
{
    static SerializePlanRegistry instance;
    return instance;
}

Expected<const SerializePlan*, String> SerializePlanRegistry::FindOrCompile(TypeId id)
{
    // 컴파일 중인 슬롯과 실패했을 때의 롤백을 다른 스레드가 보지 않도록 전체를 락 안에서 수행
    std::scoped_lock lock{ RegistrationMutex() };

    SerializePlanCompiler compiler(*this);
    auto result = compiler.Compile(id);
    if (result.HasError())
    {
        compiler.Rollback();
        return Unexpected{ String::Format("SerializePlan: cannot compile '{}': {}", DisplayNameOf(id), result.Error()) };
    }
    return result;
}
} // namespace se
