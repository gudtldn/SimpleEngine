#include "SimpleEngine/Core/Serialization/Serializer.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Memory/OsMemory.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include <utility>


namespace se
{
namespace
{
/** 최대 중첩 깊이 제한 */
constexpr usize MAX_DEPTH = 256;

/** 스택 깊이 제한을 넘었다는 오류를 archive에 남깁니다. */
void SetDepthError(Archive& archive)
{
    archive.SetError(String::Format("Serializer: exceeded the maximum nesting depth of {}.", MAX_DEPTH));
}

/** width/is_signed에 맞는 정수 타입으로 캐스팅해 value가 가리키는 메모리를 i64로 넓힙니다. */
[[nodiscard]] i64 WidenEnumValue(const void* value, EIntWidth width, bool is_signed)
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

/** i64 값 v를 width/is_signed에 맞는 정수 타입으로 좁혀 value가 가리키는 메모리에 기록합니다. */
void NarrowEnumValue(void* value, i64 v, EIntWidth width, bool is_signed)
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
 * 오류가 난 순간의 스택을 위에서부터 비우며 오류 경로를 만듭니다.
 * 자식을 처리하던 노드마다 그 자식의 표기(필드 ".name", 원소 "[i]", 맵 "[i].key" 또는 "[i].value")를 앞에 붙이고, 맨 앞의 점은 뗍니다.
 * Optional, Leaf, Enum 노드는 경로에 표기하지 않습니다.
 */
template <typename Frame>
[[nodiscard]] String TakeErrorPath(Stack<Frame>& stack)
{
    String path;
    while (const auto frame = stack.Pop())
    {
        if (!frame->has_child)
        {
            continue;
        }

        std::visit(Overloaded{
            [&](const StructSteps& steps) { path = String::Format(".{}{}", steps.fields[frame->index].name, path); },
            [&](const ArraySteps&) { path = String::Format("[{}]{}", frame->index, path); },
            [&](const SetSteps&) { path = String::Format("[{}]{}", frame->index, path); },
            [&](const MapSteps&) { path = String::Format("[{}].{}{}", frame->index / 2, frame->index % 2 == 0 ? "key" : "value", path); },
            [](const auto&) {},
        }, frame->plan->steps);
    }

    if (path.StartsWith("."))
    {
        return path.Substring(1);
    }
    return path;
}


/** 스택에 새로 쌓을, 쓸 노드 하나 */
struct WriteChild
{
    const SerializePlan* plan = nullptr;
    const void* value = nullptr;
};

/** Serialize가 스택에 쌓는 노드 하나의 진행 상태 */
struct WriteFrame
{
    const SerializePlan* plan = nullptr;
    const void* value = nullptr;
    usize index = 0;             // 처리 중이거나 다음에 처리할 자식 번호
    usize count = 0;             // 처리할 자식 수 (Array, Set, Map)
    bool begun = false;          // Begin*나 Present를 이미 썼는지
    bool has_child = false;      // index번째 자식을 처리하는 중인지. 오류 경로에 그 자식을 넣을지 정함
    Array<const void*> gathered; // Set은 원소 주소, Map은 key와 value 주소를 번갈아 모아 둠
};

/**
 * Struct 노드를 한 단계 진행합니다. 처음에는 BeginStruct를, 이후에는 필드마다 Field(name)을 쓰고 그 필드를 자식으로 돌려줍니다.
 * 필드를 다 쓰면 EndStruct를 쓰고 NullOpt를 돌려줍니다.
 */
[[nodiscard]] Optional<WriteChild> AdvanceStructWrite(ArchiveWriter& writer, WriteFrame& frame, const StructSteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        writer.BeginStruct();
        if (writer.HasError())
        {
            return NullOpt;
        }
    }
    else
    {
        frame.has_child = false;
        ++frame.index;
    }

    if (frame.index < steps.fields.Len())
    {
        const FieldStep& field = steps.fields[frame.index];
        frame.has_child = true;
        writer.Field(field.name);
        return WriteChild{ .plan = field.plan, .value = static_cast<const u8*>(frame.value) + field.offset };
    }

    writer.EndStruct();
    return NullOpt;
}

/**
 * Array 노드를 한 단계 진행합니다. 처음에는 BeginSeq(길이, Ordered)를 쓰고, 이후에는 원소를 차례로 자식으로 돌려줍니다.
 * 원소를 다 쓰면 EndSeq를 쓰고 NullOpt를 돌려줍니다.
 */
[[nodiscard]] Optional<WriteChild> AdvanceArrayWrite(ArchiveWriter& writer, WriteFrame& frame, const ArraySteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        frame.count = steps.ops->len(frame.value);
        writer.BeginSeq(frame.count, ESeqOrder::Ordered);
        if (writer.HasError())
        {
            return NullOpt;
        }
    }
    else
    {
        frame.has_child = false;
        ++frame.index;
    }

    if (frame.index < frame.count)
    {
        frame.has_child = true;
        return WriteChild{ .plan = steps.element, .value = steps.ops->element_at(frame.value, frame.index) };
    }

    writer.EndSeq();
    return NullOpt;
}

/**
 * Set 노드를 한 단계 진행합니다. 처음에는 for_each로 원소 주소를 모으고 BeginSeq(개수, Unordered)를 씁니다.
 * for_each는 중간에 멈출 수 없어 주소를 먼저 모아 두고, 이후에는 모은 원소를 차례로 자식으로 돌려줍니다. 다 쓰면 EndSeq를 씁니다.
 */
[[nodiscard]] Optional<WriteChild> AdvanceSetWrite(ArchiveWriter& writer, WriteFrame& frame, const SetSteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        frame.gathered.Reserve(steps.ops->len(frame.value));
        steps.ops->for_each(frame.value, [](const void* element, void* user_data) static
        {
            static_cast<Array<const void*>*>(user_data)->Push(element);
        }, &frame.gathered);

        frame.count = frame.gathered.Len();
        writer.BeginSeq(frame.count, ESeqOrder::Unordered);
        if (writer.HasError())
        {
            return NullOpt;
        }
    }
    else
    {
        frame.has_child = false;
        ++frame.index;
    }

    if (frame.index < frame.count)
    {
        frame.has_child = true;
        return WriteChild{ .plan = steps.element, .value = frame.gathered[frame.index] };
    }

    writer.EndSeq();
    return NullOpt;
}

/**
 * Map 노드를 한 단계 진행합니다. 처음에는 for_each로 key와 value 주소를 모으고 BeginMap을 씁니다.
 * 이후에는 엔트리마다 BeginMapEntry, key, value, EndMapEntry 순서가 되도록 key와 value를 자식으로 돌려주고, 다 쓰면 EndMap을 씁니다.
 */
[[nodiscard]] Optional<WriteChild> AdvanceMapWrite(ArchiveWriter& writer, WriteFrame& frame, const MapSteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        frame.gathered.Reserve(steps.ops->len(frame.value) * 2);
        steps.ops->for_each(frame.value, [](const void* key, const void* map_value, void* user_data) static
        {
            auto& gathered = *static_cast<Array<const void*>*>(user_data);
            gathered.Push(key);
            gathered.Push(map_value);
        }, &frame.gathered);

        frame.count = frame.gathered.Len(); // 엔트리마다 key, value 두 자식
        writer.BeginMap(frame.count / 2);
        if (writer.HasError())
        {
            return NullOpt;
        }
    }
    else
    {
        frame.has_child = false;
        if (frame.index % 2 == 1)
        {
            // value까지 썼으면 엔트리를 닫음
            writer.EndMapEntry();
            if (writer.HasError())
            {
                return NullOpt;
            }
        }
        ++frame.index;
    }

    if (frame.index < frame.count)
    {
        frame.has_child = true;
        if (frame.index % 2 == 0)
        {
            writer.BeginMapEntry();
            return WriteChild{ .plan = steps.key, .value = frame.gathered[frame.index] };
        }
        return WriteChild{ .plan = steps.value, .value = frame.gathered[frame.index] };
    }

    writer.EndMap();
    return NullOpt;
}

/** Optional 노드를 진행합니다. Present를 쓰고, 값이 있으면 내부 값을 자식으로 돌려줍니다. Optional 자체는 경로에 표기하지 않습니다. */
[[nodiscard]] Optional<WriteChild> AdvanceOptionalWrite(ArchiveWriter& writer, WriteFrame& frame, const OptionalSteps& steps)
{
    if (frame.begun)
    {
        return NullOpt; // 내부 값을 다 씀
    }

    frame.begun = true;
    const bool has_value = steps.ops->has_value(frame.value);
    writer.Present(has_value);
    if (!has_value || writer.HasError())
    {
        return NullOpt;
    }
    return WriteChild{ .plan = steps.inner, .value = steps.ops->value(frame.value) };
}

/** frame을 형태에 맞게 한 단계 진행하고, 스택에 쌓을 자식이 있으면 돌려줍니다. Leaf와 Enum은 값을 바로 씁니다. */
[[nodiscard]] Optional<WriteChild> AdvanceWrite(ArchiveWriter& writer, WriteFrame& frame)
{
    return std::visit(Overloaded{
        [&](const LeafStep& leaf) -> Optional<WriteChild>
        {
            leaf.ops.write(writer, frame.value);
            return NullOpt;
        },
        [&](const StructSteps& steps) { return AdvanceStructWrite(writer, frame, steps); },
        [&](const ArraySteps& steps) { return AdvanceArrayWrite(writer, frame, steps); },
        [&](const SetSteps& steps) { return AdvanceSetWrite(writer, frame, steps); },
        [&](const MapSteps& steps) { return AdvanceMapWrite(writer, frame, steps); },
        [&](const OptionalSteps& steps) { return AdvanceOptionalWrite(writer, frame, steps); },
        [&](const EnumStep& e) -> Optional<WriteChild>
        {
            writer.Enum(WidenEnumValue(frame.value, e.width, e.is_signed), e.width, e.is_signed, e.entries);
            return NullOpt;
        },
    }, frame.plan->steps);
}


/**
 * Set/Map을 읽는 동안 원소 하나를 잠시 만들어 두는 저장 공간
 */
class TempElement
{
public:
    TempElement() = default;
    ~TempElement() { Release(); }

    TempElement(const TempElement&) = delete;
    TempElement& operator=(const TempElement&) = delete;
    TempElement& operator=(TempElement&&) = delete;

    TempElement(TempElement&& other) noexcept
        : storage(std::exchange(other.storage, nullptr))
        , value_ops(other.value_ops)
        , constructed(std::exchange(other.constructed, false))
    {
    }

public:
    /** info의 크기와 정렬로 저장 공간을 잡습니다. */
    void Allocate(const ElementInfo& info)
    {
        storage = OsMemory::Allocate(info.size, info.alignment);
        value_ops = info.value_ops;
    }

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
    /** 만들어 둔 원소를 소멸시키고 저장 공간을 해제합니다. */
    void Release()
    {
        Destruct();
        OsMemory::Free(storage);
        storage = nullptr;
    }

private:
    void* storage = nullptr;
    const ValueOps* value_ops = nullptr;
    bool constructed = false;
};

/** 스택에 새로 쌓을, 읽을 노드 하나 */
struct ReadChild
{
    const SerializePlan* plan = nullptr;
    void* value = nullptr;
};

/** Deserialize가 스택에 쌓는 노드 하나의 진행 상태 */
struct ReadFrame
{
    const SerializePlan* plan = nullptr;
    void* value = nullptr;
    usize index = 0;        // 처리 중이거나 다음에 처리할 자식 번호
    usize count = 0;        // 처리할 자식 수 (Array, Set, Map)
    bool begun = false;     // Begin*나 Present를 이미 읽었는지
    bool has_child = false; // index번째 자식을 처리하는 중인지. 오류 경로에 그 자식을 넣을지 정함
    TempElement temp;       // Set의 원소 또는 Map의 key를 읽을 임시 원소
    TempElement temp_value; // Map의 value를 읽을 임시 원소
};

/**
 * Struct 노드를 한 단계 진행합니다. 처음에는 BeginStruct를 읽고, 이후에는 데이터에 있는 필드를 하나씩 자식으로 돌려줍니다.
 * 데이터에 없는 필드는 Optional이면 None으로 리셋하고 그 외에는 현재 값을 유지합니다. 필드를 다 처리하면 EndStruct를 읽습니다.
 */
[[nodiscard]] Optional<ReadChild> AdvanceStructRead(ArchiveReader& reader, ReadFrame& frame, const StructSteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        reader.BeginStruct();
        if (reader.HasError())
        {
            return NullOpt;
        }
    }
    else
    {
        frame.has_child = false;
        ++frame.index;
    }

    for (; frame.index < steps.fields.Len(); ++frame.index)
    {
        const FieldStep& field = steps.fields[frame.index];
        void* const field_ptr = static_cast<u8*>(frame.value) + field.offset;

        frame.has_child = true;
        if (reader.Field(field.name))
        {
            return ReadChild{ .plan = field.plan, .value = field_ptr };
        }
        if (reader.HasError())
        {
            return NullOpt; // Field() 자체의 오류는 필드 이름까지 경로에 남김
        }
        frame.has_child = false;

        // 데이터에 없는 필드: Optional은 None으로 리셋, 그 외는 현재 값 유지
        if (const auto* optional_steps = std::get_if<OptionalSteps>(&field.plan->steps))
        {
            optional_steps->ops->reset(field_ptr);
        }
    }

    reader.EndStruct();
    return NullOpt;
}

/**
 * Array 노드를 한 단계 진행합니다. 처음에는 BeginSeq를 읽고, resize가 있으면 비운 뒤 읽은 길이로 다시 만듭니다(교체).
 * resize가 nullptr이면(FixedArray이거나 원소를 기본 생성할 수 없음) 길이가 이미 같을 때만 제자리에서 읽고, 다르면 오류입니다.
 * 이후에는 원소를 차례로 자식으로 돌려주고, 다 읽으면 EndSeq를 읽습니다.
 */
[[nodiscard]] Optional<ReadChild> AdvanceArrayRead(ArchiveReader& reader, ReadFrame& frame, const ArraySteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        u64 count = 0;
        reader.BeginSeq(count);
        if (reader.HasError())
        {
            return NullOpt;
        }

        if (steps.ops->resize != nullptr)
        {
            steps.ops->resize(frame.value, 0);
            steps.ops->resize(frame.value, static_cast<usize>(count));
        }
        else if (steps.ops->len(frame.value) != static_cast<usize>(count))
        {
            reader.SetError(String::Format(
                "Deserialize: array length is fixed at {} and cannot be resized to {}.",
                steps.ops->len(frame.value), count));
            return NullOpt;
        }
        frame.count = steps.ops->len(frame.value);
    }
    else
    {
        frame.has_child = false;
        ++frame.index;
    }

    if (frame.index < frame.count)
    {
        frame.has_child = true;
        return ReadChild{ .plan = steps.element, .value = steps.ops->element_at_mut(frame.value, frame.index) };
    }

    reader.EndSeq();
    return NullOpt;
}

/**
 * Set 노드를 한 단계 진행합니다. 처음에는 BeginSeq를 읽고 기존 원소를 clear합니다(교체).
 * 이후에는 임시 원소를 기본 생성해 자식으로 돌려주고, 다 읽은 임시 원소는 emplace_moved로 옮겨 넣은 뒤 소멸시킵니다. 다 읽으면 EndSeq를 읽습니다.
 */
[[nodiscard]] Optional<ReadChild> AdvanceSetRead(ArchiveReader& reader, ReadFrame& frame, const SetSteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        u64 count = 0;
        reader.BeginSeq(count);
        if (reader.HasError())
        {
            return NullOpt;
        }

        steps.ops->clear(frame.value);
        frame.count = static_cast<usize>(count);
        if (frame.count > 0)
        {
            // 임시 원소가 필요할 때만 op 검사
            if (steps.element_info.value_ops->default_construct_at == nullptr)
            {
                reader.SetError("Deserialize: set element type has no default constructor to build a temporary element.");
                return NullOpt;
            }
            if (steps.ops->emplace_moved == nullptr)
            {
                reader.SetError("Deserialize: set element type has no move constructor to insert a temporary element.");
                return NullOpt;
            }
            frame.temp.Allocate(steps.element_info);
        }
    }
    else
    {
        // 다 읽은 임시 원소를 Set에 옮겨 넣음
        frame.has_child = false;
        steps.ops->emplace_moved(frame.value, frame.temp.Get());
        frame.temp.Destruct();
        ++frame.index;
    }

    if (frame.index < frame.count)
    {
        frame.has_child = true;
        frame.temp.Construct();
        return ReadChild{ .plan = steps.element, .value = frame.temp.Get() };
    }

    reader.EndSeq();
    return NullOpt;
}

/**
 * Map 노드를 한 단계 진행합니다. 처음에는 BeginMap을 읽고 기존 엔트리를 clear합니다(교체).
 * 이후에는 엔트리마다 BeginMapEntry를 읽고 임시 key와 value를 차례로 자식으로 돌려준 뒤, EndMapEntry를 읽고 emplace_moved로 옮겨 넣습니다.
 * 다 읽으면 EndMap을 읽습니다.
 */
[[nodiscard]] Optional<ReadChild> AdvanceMapRead(ArchiveReader& reader, ReadFrame& frame, const MapSteps& steps)
{
    if (!frame.begun)
    {
        frame.begun = true;
        u64 count = 0;
        reader.BeginMap(count);
        if (reader.HasError())
        {
            return NullOpt;
        }

        steps.ops->clear(frame.value);
        frame.count = static_cast<usize>(count) * 2; // 엔트리마다 key, value 두 자식
        if (frame.count > 0)
        {
            // 임시 원소가 필요할 때만 op 검사
            if (steps.key_info.value_ops->default_construct_at == nullptr)
            {
                reader.SetError("Deserialize: map key type has no default constructor to build a temporary element.");
                return NullOpt;
            }
            if (steps.value_info.value_ops->default_construct_at == nullptr)
            {
                reader.SetError("Deserialize: map value type has no default constructor to build a temporary element.");
                return NullOpt;
            }
            if (steps.ops->emplace_moved == nullptr)
            {
                reader.SetError("Deserialize: map entry type has no move constructor to insert a temporary key/value.");
                return NullOpt;
            }
            frame.temp.Allocate(steps.key_info);
            frame.temp_value.Allocate(steps.value_info);
        }
    }
    else
    {
        frame.has_child = false;
        if (frame.index % 2 == 0)
        {
            // key를 읽었으면 같은 엔트리의 value를 읽음
            ++frame.index;
            frame.has_child = true;
            return ReadChild{ .plan = steps.value, .value = frame.temp_value.Get() };
        }

        // value까지 읽었으면 엔트리를 닫고 Map에 옮겨 넣음
        reader.EndMapEntry();
        if (reader.HasError())
        {
            return NullOpt;
        }
        steps.ops->emplace_moved(frame.value, frame.temp.Get(), frame.temp_value.Get());
        frame.temp.Destruct();
        frame.temp_value.Destruct();
        ++frame.index;
    }

    if (frame.index < frame.count)
    {
        frame.has_child = true;
        reader.BeginMapEntry();
        if (reader.HasError())
        {
            return NullOpt;
        }
        frame.temp.Construct();
        frame.temp_value.Construct();
        return ReadChild{ .plan = steps.key, .value = frame.temp.Get() };
    }

    reader.EndMap();
    return NullOpt;
}

/**
 * Optional 노드를 진행합니다. Present를 읽어 값이 없으면 reset하고, 있으면 emplace한 자리를 내부 값 자식으로 돌려줍니다.
 * Optional 자체는 경로에 표기하지 않습니다.
 */
[[nodiscard]] Optional<ReadChild> AdvanceOptionalRead(ArchiveReader& reader, ReadFrame& frame, const OptionalSteps& steps)
{
    if (frame.begun)
    {
        return NullOpt; // 내부 값을 다 읽음
    }

    frame.begun = true;
    bool has_value = false;
    reader.Present(has_value);
    if (reader.HasError())
    {
        return NullOpt;
    }

    if (!has_value)
    {
        steps.ops->reset(frame.value);
        return NullOpt;
    }
    if (steps.ops->emplace == nullptr)
    {
        reader.SetError("Deserialize: optional inner type cannot be emplaced (e.g. Optional<T&>).");
        return NullOpt;
    }
    return ReadChild{ .plan = steps.inner, .value = steps.ops->emplace(frame.value) };
}

/** frame을 형태에 맞게 한 단계 진행하고, 스택에 쌓을 자식이 있으면 돌려줍니다. Leaf와 Enum은 값을 바로 읽습니다. */
[[nodiscard]] Optional<ReadChild> AdvanceRead(ArchiveReader& reader, ReadFrame& frame)
{
    return std::visit(Overloaded{
        [&](const LeafStep& leaf) -> Optional<ReadChild>
        {
            leaf.ops.read(reader, frame.value);
            return NullOpt;
        },
        [&](const StructSteps& steps) { return AdvanceStructRead(reader, frame, steps); },
        [&](const ArraySteps& steps) { return AdvanceArrayRead(reader, frame, steps); },
        [&](const SetSteps& steps) { return AdvanceSetRead(reader, frame, steps); },
        [&](const MapSteps& steps) { return AdvanceMapRead(reader, frame, steps); },
        [&](const OptionalSteps& steps) { return AdvanceOptionalRead(reader, frame, steps); },
        [&](const EnumStep& e) -> Optional<ReadChild>
        {
            i64 v = 0;
            reader.Enum(v, e.width, e.is_signed, e.entries);
            if (!reader.HasError())
            {
                NarrowEnumValue(frame.value, v, e.width, e.is_signed);
            }
            return NullOpt;
        },
    }, frame.plan->steps);
}
} // namespace

Expected<void, SerializeError> Serialize(ArchiveWriter& writer, const SerializePlan& plan, const void* value)
{
    Stack<WriteFrame> stack;
    stack.Push(WriteFrame{ .plan = &plan, .value = value });

    while (!writer.HasError() && !stack.IsEmpty())
    {
        const auto child = AdvanceWrite(writer, *stack.Peek());
        if (writer.HasError())
        {
            break;
        }

        if (!child.HasValue())
        {
            stack.Pop();
        }
        else if (stack.Len() >= MAX_DEPTH)
        {
            SetDepthError(writer);
        }
        else
        {
            stack.Push(WriteFrame{ .plan = child->plan, .value = child->value });
        }
    }

    if (writer.HasError())
    {
        return Unexpected{ SerializeError{ .path = TakeErrorPath(stack), .message = String(writer.GetError()) } };
    }
    return {};
}

Expected<void, SerializeError> Deserialize(ArchiveReader& reader, const SerializePlan& plan, void* value)
{
    Stack<ReadFrame> stack;
    stack.Push(ReadFrame{ .plan = &plan, .value = value });

    while (!reader.HasError() && !stack.IsEmpty())
    {
        const auto child = AdvanceRead(reader, stack.Peek().Value());
        if (reader.HasError())
        {
            break;
        }

        if (!child.HasValue())
        {
            stack.Pop();
        }
        else if (stack.Len() >= MAX_DEPTH)
        {
            SetDepthError(reader);
        }
        else
        {
            stack.Push(ReadFrame{ .plan = child->plan, .value = child->value });
        }
    }

    if (reader.HasError())
    {
        return Unexpected{ SerializeError{ .path = TakeErrorPath(stack), .message = String(reader.GetError()) } };
    }
    return {};
}
} // namespace se
