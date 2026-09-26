#include "SimpleEngine/Core/Serialization/Serializer.h"

#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include <utility>


namespace se
{
namespace
{
/** 값이 중첩될 수 있는 최대 깊이 */
constexpr usize MAX_NESTING_DEPTH = 256;

/** depth가 MAX_NESTING_DEPTH를 넘으면 archive에 오류를 남기고 true를 돌려줍니다. */
[[nodiscard]] bool ExceedsMaxDepth(Archive& archive, usize depth)
{
    if (depth > MAX_NESTING_DEPTH)
    {
        archive.SetError(String::Format("Serializer: exceeded the maximum nesting depth of {}.", MAX_NESTING_DEPTH));
        return true;
    }
    return false;
}

/** width/is_signed에 맞는 정수 타입으로 value가 가리키는 enum 값을 꺼내 i64로 돌려줍니다. */
[[nodiscard]] i64 LoadEnumValue(const void* value, EIntWidth width, bool is_signed)
{
    switch (width)
    {
    case EIntWidth::Bits8: return is_signed ? static_cast<i64>(*static_cast<const i8*>(value)) : static_cast<i64>(*static_cast<const u8*>(value));
    case EIntWidth::Bits16: return is_signed ? static_cast<i64>(*static_cast<const i16*>(value)) : static_cast<i64>(*static_cast<const u16*>(value));
    case EIntWidth::Bits32: return is_signed ? static_cast<i64>(*static_cast<const i32*>(value)) : static_cast<i64>(*static_cast<const u32*>(value));
    case EIntWidth::Bits64: return is_signed ? static_cast<i64>(*static_cast<const i64*>(value)) : static_cast<i64>(*static_cast<const u64*>(value));
    }
    SE_UNREACHABLE();
}

/** i64 값 v를 width/is_signed에 맞는 정수 타입으로 좁혀 value가 가리키는 enum에 넣습니다. */
void StoreEnumValue(void* value, i64 v, EIntWidth width, bool is_signed)
{
    switch (width)
    {
    case EIntWidth::Bits8:
        if (is_signed) { *static_cast<i8*>(value) = static_cast<i8>(v); }
        else { *static_cast<u8*>(value) = static_cast<u8>(v); }
        return;
    case EIntWidth::Bits16:
        if (is_signed) { *static_cast<i16*>(value) = static_cast<i16>(v); }
        else { *static_cast<u16*>(value) = static_cast<u16>(v); }
        return;
    case EIntWidth::Bits32:
        if (is_signed) { *static_cast<i32*>(value) = static_cast<i32>(v); }
        else { *static_cast<u32*>(value) = static_cast<u32>(v); }
        return;
    case EIntWidth::Bits64:
        if (is_signed) { *static_cast<i64*>(value) = static_cast<i64>(v); } // NOLINT(*-redundant-casting)
        else { *static_cast<u64*>(value) = static_cast<u64>(v); }
        return;
    }
    SE_UNREACHABLE();
}

/**
 * 오류가 난 값까지의 경로를, 재귀를 빠져나오며 안쪽부터 앞으로 붙여 만듭니다.
 * Optional, Leaf, Enum 노드는 경로에 표기하지 않습니다.
 */
class ErrorPathBuilder
{
public:
    /** 필드 표기 ".name"을 앞에 붙입니다. */
    void PrependField(StringView name) { Prepend(String::Format(".{}", name)); }

    /** 원소 표기 "[index]"를 앞에 붙입니다. */
    void PrependElement(usize index) { Prepend(String::Format("[{}]", index)); }

    /** 맵 엔트리 key 표기 "[index].key"를 앞에 붙입니다. */
    void PrependMapKey(usize index) { Prepend(String::Format("[{}].key", index)); }

    /** 맵 엔트리 value 표기 "[index].value"를 앞에 붙입니다. */
    void PrependMapValue(usize index) { Prepend(String::Format("[{}].value", index)); }

    /** 완성된 경로를 돌려줍니다. 맨 앞의 점은 뗍니다. */
    [[nodiscard]] String Build()
    {
        if (path.StartsWith("."))
        {
            return path.Substring(1);
        }
        return std::move(path);
    }

private:
    void Prepend(StringView segment)
    {
        path = String::Format("{}{}", segment, path);
    }

private:
    String path;
};


/**
 * plan을 따라 value와 그 안의 값을 재귀로 writer에 씁니다.
 * writer에 오류가 생기면 더 쓰지 않고 빠져나옵니다.
 */
class ValueWriter
{
public:
    explicit ValueWriter(ArchiveWriter& in_writer)
        : writer(in_writer)
    {
    }

public:
    /** value를 형태에 맞게 씁니다. depth는 value 자신을 포함한 중첩 깊이입니다. */
    void Write(const SerializePlan& plan, const void* value, usize depth)
    {
        if (writer.HasError() || ExceedsMaxDepth(writer, depth))
        {
            return;
        }

        std::visit(Overloaded{
            [&](const LeafStep& leaf) { leaf.ops.write(writer, value); },
            [&](const StructSteps& steps) { WriteStruct(steps, value, depth); },
            [&](const ArraySteps& steps) { WriteArray(steps, value, depth); },
            [&](const SetSteps& steps) { WriteSet(steps, value, depth); },
            [&](const MapSteps& steps) { WriteMap(steps, value, depth); },
            [&](const OptionalSteps& steps) { WriteOptional(steps, value, depth); },
            [&](const EnumStep& enum_step)
            {
                writer.Enum(LoadEnumValue(value, enum_step.width, enum_step.is_signed), enum_step.width, enum_step.is_signed, enum_step.entries);
            },
        }, plan.steps);
    }

    /** 오류가 난 값까지의 경로를 돌려줍니다. */
    [[nodiscard]] String BuildErrorPath() { return error_path.Build(); }

private:
    /** BeginStruct를 쓰고, 필드마다 Field(name)과 필드 값을 쓴 뒤 EndStruct를 씁니다. */
    void WriteStruct(const StructSteps& steps, const void* value, usize depth)
    {
        writer.BeginStruct();
        if (writer.HasError())
        {
            return;
        }

        for (const FieldStep& field : steps.fields)
        {
            writer.Field(field.name);
            Write(*field.plan, static_cast<const u8*>(value) + field.offset, depth + 1);
            if (writer.HasError())
            {
                error_path.PrependField(field.name);
                return;
            }
        }
        writer.EndStruct();
    }

    /** BeginSeq(길이, Ordered)를 쓰고, 원소를 차례로 쓴 뒤 EndSeq를 씁니다. */
    void WriteArray(const ArraySteps& steps, const void* value, usize depth)
    {
        const usize count = steps.ops->len(value);
        writer.BeginSeq(count, ESeqOrder::Ordered);
        if (writer.HasError())
        {
            return;
        }

        for (usize i = 0; i < count; ++i)
        {
            Write(*steps.element, steps.ops->element_at(value, i), depth + 1);
            if (writer.HasError())
            {
                error_path.PrependElement(i);
                return;
            }
        }
        writer.EndSeq();
    }

    /**
     * BeginSeq(개수, Unordered)를 쓰고, for_each로 원소를 차례로 쓴 뒤 EndSeq를 씁니다.
     * for_each는 중간에 멈출 수 없어, 오류가 난 뒤의 원소는 건너뜁니다.
     */
    void WriteSet(const SetSteps& steps, const void* value, usize depth)
    {
        writer.BeginSeq(steps.ops->len(value), ESeqOrder::Unordered);

        struct SetCallbackData
        {
            ValueWriter& self;
            const SerializePlan& element;
            usize depth;
            usize index = 0;
        };
        SetCallbackData callback_data{ .self = *this, .element = *steps.element, .depth = depth };

        steps.ops->for_each(value, [](const void* element, void* user_data) static
        {
            auto& data = *static_cast<SetCallbackData*>(user_data);
            if (data.self.writer.HasError())
            {
                return;
            }

            data.self.Write(data.element, element, data.depth + 1);
            if (data.self.writer.HasError())
            {
                data.self.error_path.PrependElement(data.index);
            }
            ++data.index;
        }, &callback_data);

        writer.EndSeq();
    }

    /**
     * BeginMap을 쓰고, for_each로 엔트리마다 BeginMapEntry, key, value, EndMapEntry를 쓴 뒤 EndMap을 씁니다.
     * for_each는 중간에 멈출 수 없어, 오류가 난 뒤의 엔트리는 건너뜁니다.
     */
    void WriteMap(const MapSteps& steps, const void* value, usize depth)
    {
        writer.BeginMap(steps.ops->len(value));

        struct MapCallbackData
        {
            ValueWriter& self;
            const MapSteps& steps;
            usize depth;
            usize index = 0;
        };
        MapCallbackData callback_data{ .self = *this, .steps = steps, .depth = depth };

        steps.ops->for_each(value, [](const void* key, const void* map_value, void* user_data) static
        {
            auto& data = *static_cast<MapCallbackData*>(user_data);
            if (data.self.writer.HasError())
            {
                return;
            }

            // BeginMapEntry의 오류도 key 경로에 남김
            data.self.writer.BeginMapEntry();
            data.self.Write(*data.steps.key, key, data.depth + 1);
            if (data.self.writer.HasError())
            {
                data.self.error_path.PrependMapKey(data.index);
                return;
            }

            data.self.Write(*data.steps.value, map_value, data.depth + 1);
            if (data.self.writer.HasError())
            {
                data.self.error_path.PrependMapValue(data.index);
                return;
            }

            data.self.writer.EndMapEntry();
            ++data.index;
        }, &callback_data);

        writer.EndMap();
    }

    /** Present를 쓰고, 값이 있으면 내부 값을 씁니다. */
    void WriteOptional(const OptionalSteps& steps, const void* value, usize depth)
    {
        const bool has_value = steps.ops->has_value(value);
        writer.Present(has_value);
        if (has_value)
        {
            Write(*steps.inner, steps.ops->value(value), depth + 1);
        }
    }

private:
    ArchiveWriter& writer;
    ErrorPathBuilder error_path;
};


/**
 * Set/Map을 읽는 동안 원소 하나를 잠시 만들어 두는 저장 공간
 * 생성할 때 저장 공간을 잡고, 파괴할 때 남은 원소를 소멸시키고 해제합니다.
 */
class TempElement
{
public:
    explicit TempElement(const ElementInfo& info)
        : storage(OsMemory::Allocate(info.size, info.alignment))
        , value_ops(info.value_ops)
    {
    }

    ~TempElement()
    {
        Destruct();
        OsMemory::Free(storage);
    }

    TempElement(const TempElement&) = delete;
    TempElement& operator=(const TempElement&) = delete;

public:
    /** 저장 공간에 원소를 기본 생성합니다. */
    void Construct()
    {
        value_ops->default_construct_at(storage);
        constructed = true;
    }

    /** 만들어 둔 원소를 소멸시킵니다. 저장 공간은 다음 원소에 다시 씁니다. */
    void Destruct()
    {
        if (constructed)
        {
            value_ops->destruct_at(storage);
            constructed = false;
        }
    }

    /** 저장 공간의 주소 */
    [[nodiscard]] void* Get() const { return storage; }

private:
    void* storage = nullptr;
    const ValueOps* value_ops = nullptr;
    bool constructed = false;
};


/**
 * plan을 따라 reader에서 읽은 값을 value와 그 안의 값에 재귀로 채웁니다.
 * reader에 오류가 생기면 더 읽지 않고 빠져나옵니다.
 */
class ValueReader
{
public:
    explicit ValueReader(ArchiveReader& in_reader)
        : reader(in_reader)
    {
    }

public:
    /** value를 형태에 맞게 읽습니다. depth는 value 자신을 포함한 중첩 깊이입니다. */
    void Read(const SerializePlan& plan, void* value, usize depth)
    {
        if (reader.HasError() || ExceedsMaxDepth(reader, depth))
        {
            return;
        }

        std::visit(Overloaded{
            [&](const LeafStep& leaf) { leaf.ops.read(reader, value); },
            [&](const StructSteps& steps) { ReadStruct(steps, value, depth); },
            [&](const ArraySteps& steps) { ReadArray(steps, value, depth); },
            [&](const SetSteps& steps) { ReadSet(steps, value, depth); },
            [&](const MapSteps& steps) { ReadMap(steps, value, depth); },
            [&](const OptionalSteps& steps) { ReadOptional(steps, value, depth); },
            [&](const EnumStep& enum_step)
            {
                i64 v = 0;
                reader.Enum(v, enum_step.width, enum_step.is_signed, enum_step.entries);
                if (!reader.HasError())
                {
                    StoreEnumValue(value, v, enum_step.width, enum_step.is_signed);
                }
            },
        }, plan.steps);
    }

    /** 오류가 난 값까지의 경로를 돌려줍니다. */
    [[nodiscard]] String BuildErrorPath() { return error_path.Build(); }

private:
    /**
     * BeginStruct를 읽고, 데이터에 있는 필드를 읽은 뒤 EndStruct를 읽습니다.
     * 데이터에 없는 필드는 Optional이면 None으로 리셋하고 그 외에는 현재 값을 유지합니다.
     */
    void ReadStruct(const StructSteps& steps, void* value, usize depth)
    {
        reader.BeginStruct();
        if (reader.HasError())
        {
            return;
        }

        for (const FieldStep& field : steps.fields)
        {
            void* const field_value = static_cast<u8*>(value) + field.offset;

            const bool present = reader.Field(field.name);
            if (present)
            {
                Read(*field.plan, field_value, depth + 1);
            }
            if (reader.HasError())
            {
                // Field() 자체의 오류도 필드 이름까지 경로에 남김
                error_path.PrependField(field.name);
                return;
            }

            if (!present)
            {
                if (const auto* optional_steps = std::get_if<OptionalSteps>(&field.plan->steps))
                {
                    optional_steps->ops->reset(field_value);
                }
            }
        }
        reader.EndStruct();
    }

    /**
     * BeginSeq를 읽고, resize가 있으면 비운 뒤 읽은 길이로 다시 만듭니다(교체).
     * resize가 nullptr이면(FixedArray이거나 원소를 기본 생성할 수 없음) 길이가 이미 같을 때만 제자리에서 읽고, 다르면 오류입니다.
     * 원소를 차례로 읽은 뒤 EndSeq를 읽습니다.
     */
    void ReadArray(const ArraySteps& steps, void* value, usize depth)
    {
        u64 count = 0;
        reader.BeginSeq(count);
        if (reader.HasError())
        {
            return;
        }

        if (steps.ops->resize != nullptr)
        {
            steps.ops->resize(value, 0);
            steps.ops->resize(value, static_cast<usize>(count));
        }
        else if (steps.ops->len(value) != static_cast<usize>(count))
        {
            reader.SetError(String::Format(
                "Deserialize: array length is fixed at {} and cannot be resized to {}.",
                steps.ops->len(value), count));
            return;
        }

        const usize len = steps.ops->len(value);
        for (usize i = 0; i < len; ++i)
        {
            Read(*steps.element, steps.ops->element_at_mut(value, i), depth + 1);
            if (reader.HasError())
            {
                error_path.PrependElement(i);
                return;
            }
        }
        reader.EndSeq();
    }

    /**
     * BeginSeq를 읽고 기존 원소를 clear합니다(교체).
     * 원소마다 임시 원소를 기본 생성해 읽고, emplace_moved로 옮겨 넣은 뒤 소멸시킵니다. 다 읽으면 EndSeq를 읽습니다.
     */
    void ReadSet(const SetSteps& steps, void* value, usize depth)
    {
        u64 count = 0;
        reader.BeginSeq(count);
        if (reader.HasError())
        {
            return;
        }

        steps.ops->clear(value);
        if (count == 0)
        {
            // 빈 Set은 임시 원소가 필요 없음
            reader.EndSeq();
            return;
        }

        if (steps.element_info.value_ops->default_construct_at == nullptr)
        {
            reader.SetError("Deserialize: set element type has no default constructor to build a temporary element.");
            return;
        }
        if (steps.ops->emplace_moved == nullptr)
        {
            reader.SetError("Deserialize: set element type has no move constructor to insert a temporary element.");
            return;
        }

        TempElement element(steps.element_info);
        for (usize i = 0; i < static_cast<usize>(count); ++i)
        {
            element.Construct();
            Read(*steps.element, element.Get(), depth + 1);
            if (reader.HasError())
            {
                error_path.PrependElement(i);
                return;
            }
            steps.ops->emplace_moved(value, element.Get());
            element.Destruct();
        }
        reader.EndSeq();
    }

    /**
     * BeginMap을 읽고 기존 엔트리를 clear합니다(교체).
     * 엔트리마다 BeginMapEntry를 읽고 임시 key와 value를 읽은 뒤, EndMapEntry를 읽고 emplace_moved로 옮겨 넣습니다. 다 읽으면 EndMap을 읽습니다.
     */
    void ReadMap(const MapSteps& steps, void* value, usize depth)
    {
        u64 count = 0;
        reader.BeginMap(count);
        if (reader.HasError())
        {
            return;
        }

        steps.ops->clear(value);
        if (count == 0)
        {
            // 빈 Map은 임시 key와 value가 필요 없음
            reader.EndMap();
            return;
        }

        if (steps.key_info.value_ops->default_construct_at == nullptr)
        {
            reader.SetError("Deserialize: map key type has no default constructor to build a temporary element.");
            return;
        }
        if (steps.value_info.value_ops->default_construct_at == nullptr)
        {
            reader.SetError("Deserialize: map value type has no default constructor to build a temporary element.");
            return;
        }
        if (steps.ops->emplace_moved == nullptr)
        {
            reader.SetError("Deserialize: map entry type has no move constructor to insert a temporary key/value.");
            return;
        }

        TempElement key(steps.key_info);
        TempElement map_value(steps.value_info);
        for (usize i = 0; i < static_cast<usize>(count); ++i)
        {
            // BeginMapEntry의 오류도 key 경로에 남김
            reader.BeginMapEntry();
            if (reader.HasError())
            {
                error_path.PrependMapKey(i);
                return;
            }

            key.Construct();
            map_value.Construct();
            Read(*steps.key, key.Get(), depth + 1);
            if (reader.HasError())
            {
                error_path.PrependMapKey(i);
                return;
            }
            Read(*steps.value, map_value.Get(), depth + 1);
            if (reader.HasError())
            {
                error_path.PrependMapValue(i);
                return;
            }

            reader.EndMapEntry();
            if (reader.HasError())
            {
                return;
            }
            steps.ops->emplace_moved(value, key.Get(), map_value.Get());
            key.Destruct();
            map_value.Destruct();
        }
        reader.EndMap();
    }

    /** Present를 읽어 값이 없으면 reset하고, 있으면 emplace한 자리에 내부 값을 읽습니다. */
    void ReadOptional(const OptionalSteps& steps, void* value, usize depth)
    {
        bool has_value = false;
        reader.Present(has_value);
        if (reader.HasError())
        {
            return;
        }

        if (!has_value)
        {
            steps.ops->reset(value);
            return;
        }
        if (steps.ops->emplace == nullptr)
        {
            reader.SetError("Deserialize: optional inner type cannot be emplaced (e.g. Optional<T&>).");
            return;
        }
        Read(*steps.inner, steps.ops->emplace(value), depth + 1);
    }

private:
    ArchiveReader& reader;
    ErrorPathBuilder error_path;
};
} // namespace

namespace serde
{
Expected<void, SerializeError> Serialize(ArchiveWriter& writer, const SerializePlan& plan, const void* value)
{
    ValueWriter value_writer(writer);
    value_writer.Write(plan, value, 1);

    if (writer.HasError())
    {
        return Unexpected{ SerializeError{ .path = value_writer.BuildErrorPath(), .message = String(writer.GetError()) } };
    }
    return {};
}

Expected<void, SerializeError> Deserialize(ArchiveReader& reader, const SerializePlan& plan, void* value)
{
    ValueReader value_reader(reader);
    value_reader.Read(plan, value, 1);

    if (reader.HasError())
    {
        return Unexpected{ SerializeError{ .path = value_reader.BuildErrorPath(), .message = String(reader.GetError()) } };
    }
    return {};
}
} // namespace serde
} // namespace se
