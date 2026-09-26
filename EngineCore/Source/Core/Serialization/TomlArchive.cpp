#include "SimpleEngine/Core/Serialization/TomlArchive.h"

#include "SimpleEngine/Utility/Base64.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include <algorithm>
#include <charconv>
#include <compare>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>


namespace se
{
namespace
{
/** Int/Enum 노드의 폭과 부호를 오류 메시지에 쓸 타입 이름으로 바꿉니다. */
[[nodiscard]] StringView IntTypeName(EIntWidth width, bool is_signed)
{
    switch (width)
    {
    case EIntWidth::Bits8: return is_signed ? "i8" : "u8";
    case EIntWidth::Bits16: return is_signed ? "i16" : "u16";
    case EIntWidth::Bits32: return is_signed ? "i32" : "u32";
    case EIntWidth::Bits64: return is_signed ? "i64" : "u64";
    }
    SE_UNREACHABLE();
}

/**
 * value가 width와 부호로 표현할 수 있는 범위 안인지 확인합니다.
 * u64는 0 이상의 i64만 받습니다. i64를 넘는 u64는 10진 문자열로 따로 읽습니다.
 */
[[nodiscard]] bool FitsInWidth(i64 value, EIntWidth width, bool is_signed)
{
    switch (width)
    {
    case EIntWidth::Bits8: return is_signed ? std::in_range<i8>(value) : std::in_range<u8>(value);
    case EIntWidth::Bits16: return is_signed ? std::in_range<i16>(value) : std::in_range<u16>(value);
    case EIntWidth::Bits32: return is_signed ? std::in_range<i32>(value) : std::in_range<u32>(value);
    case EIntWidth::Bits64: return is_signed || value >= 0;
    }
    SE_UNREACHABLE();
}

/** 오류 메시지에 쓸 TOML 노드 종류의 이름을 돌려줍니다. */
[[nodiscard]] StringView NodeKindName(const toml::node& node)
{
    switch (node.type())
    {
    case toml::node_type::table:          return "a table";
    case toml::node_type::array:          return "an array";
    case toml::node_type::string:         return "a string";
    case toml::node_type::integer:        return "an integer";
    case toml::node_type::floating_point: return "a float";
    case toml::node_type::boolean:        return "a boolean";
    case toml::node_type::date:           return "a date";
    case toml::node_type::time:           return "a time";
    case toml::node_type::date_time:      return "a date-time";
    case toml::node_type::none:           break;
    }
    return "an empty node";
}

/** toml++의 키와 문자열 값으로 넘길 수 있게 std::string_view로 바꿉니다. */
[[nodiscard]] std::string_view ToStdStringView(StringView text)
{
    return { text };
}

/** TOML 위치 parent 아래에 있는 key의 위치를 만듭니다. 예: ("window", "width")는 "window.width", ("", "vfs")는 "vfs" */
[[nodiscard]] String JoinPath(StringView parent, StringView key)
{
    if (parent.IsEmpty())
    {
        return { key };
    }
    return String::Format("{}.{}", parent, key);
}

/**
 * f32 값을 TOML에 저장할 f64로 바꿉니다. f32로 되읽히는 가장 짧은 10진 표현을 씁니다(0.1f는 0.1).
 * 그 표현을 f64로 읽은 값이 f32로 돌아오지 않으면 확장된 값을 그대로 씁니다.
 */
[[nodiscard]] f64 ToShortestF64(f32 value)
{
    if (!std::isfinite(value))
    {
        return value;
    }

    char buffer[32];
    const char* const end = std::to_chars(std::begin(buffer), std::end(buffer), value).ptr;
    f64 shortest = 0.0;
    std::from_chars(buffer, end, shortest);

    // 짧은 표현이 두 f32의 정확한 중간값으로 읽히면 짝수 쪽 이웃으로 반올림됨 (유한 f32 전체 중 2개)
    if (static_cast<f32>(shortest) != value)
    {
        return value;
    }
    return shortest;
}

/** node를 toml++ 기본 형식의 텍스트로 만듭니다. */
[[nodiscard]] std::string ToTomlText(const toml::node& node)
{
    std::ostringstream stream;
    node.visit([&stream](const auto& concrete) { stream << concrete; });
    return std::move(stream).str();
}

/**
 * 순서 없는 출력을 정렬할 때 쓰는 전순서로 두 노드를 비교합니다.
 * 종류가 다르면 toml++의 종류 순서이고, 같으면 정수와 실수는 값, 문자열은 사전순, bool은 false가 먼저입니다.
 * 배열은 원소를 앞에서부터 비교하고([key, value] 쌍은 key가 먼저), 테이블은 TOML 텍스트로 비교합니다.
 */
[[nodiscard]] std::weak_ordering CompareNodes(const toml::node& lhs, const toml::node& rhs) // NOLINT(*-no-recursion)
{
    if (lhs.type() != rhs.type())
    {
        return lhs.type() <=> rhs.type();
    }

    switch (lhs.type())
    {
    case toml::node_type::integer:        return lhs.as_integer()->get() <=> rhs.as_integer()->get();
    case toml::node_type::floating_point: return std::strong_order(lhs.as_floating_point()->get(), rhs.as_floating_point()->get());
    case toml::node_type::string:         return lhs.as_string()->get() <=> rhs.as_string()->get();
    case toml::node_type::boolean:        return lhs.as_boolean()->get() <=> rhs.as_boolean()->get();
    case toml::node_type::array:
    {
        const toml::array& lhs_array = *lhs.as_array();
        const toml::array& rhs_array = *rhs.as_array();
        return std::lexicographical_compare_three_way(
            lhs_array.begin(), lhs_array.end(), rhs_array.begin(), rhs_array.end(), CompareNodes);
    }
    default:                              return ToTomlText(lhs) <=> ToTomlText(rhs);
    }
}

/**
 * array의 원소를 CompareNodes 순서로 정렬합니다.
 * toml::array는 원소를 제자리에서 맞바꿀 수 없어, 정렬한 순서로 옮겨 담은 새 배열로 교체합니다.
 */
void SortArray(toml::array& array)
{
    Array<toml::node*> order;
    order.Reserve(array.size());
    for (toml::node& element : array)
    {
        order.Push(&element);
    }
    order.Sort([](const toml::node* lhs, const toml::node* rhs) { return CompareNodes(*lhs, *rhs) < 0; });

    toml::array sorted;
    sorted.reserve(array.size());
    for (toml::node* const element : order)
    {
        sorted.push_back(std::move(*element));
    }
    array = std::move(sorted);
}
} // namespace


// TomlWriter
TomlWriter::TomlWriter(toml::table& out_root)
    : root(out_root)
{
}

bool TomlWriter::IsTextFormat() const
{
    return true;
}

void TomlWriter::Int(i64 value, EIntWidth width, bool is_signed)
{
    // TOML 정수는 i64라서 i64를 넘는 u64는 10진 문자열로 씀
    if (width == EIntWidth::Bits64 && !is_signed && value < 0)
    {
        PlaceValue(std::to_string(static_cast<u64>(value)));
        return;
    }
    PlaceValue(value);
}

void TomlWriter::Float(f64 value, EFloatWidth width)
{
    if (width == EFloatWidth::Bits32)
    {
        PlaceValue(ToShortestF64(static_cast<f32>(value)));
        return;
    }
    PlaceValue(value);
}

void TomlWriter::Bool(bool value)
{
    PlaceValue(value);
}

void TomlWriter::Str(StringView value)
{
    PlaceValue(ToStdStringView(value));
}

void TomlWriter::Bytes(const void* data, u64 size)
{
    const String text = base64::Encode(ArrayView<const u8>(static_cast<const u8*>(data), size));
    PlaceValue(ToStdStringView(text));
}

void TomlWriter::Enum(i64 value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries)
{
    // 이름이 있는 값은 이름으로, 없는 값(플래그 조합 등)은 정수로 씀
    const auto* const entry = std::ranges::find(entries, value, &EnumEntry::value);
    if (entry != entries.end())
    {
        PlaceValue(ToStdStringView(entry->name));
        return;
    }

    // Int처럼 문자열로 쓰면 이름으로 읽혀 되읽을 수 없으므로 오류
    if (width == EIntWidth::Bits64 && !is_signed && value < 0)
    {
        SetError(String::Format("TomlWriter: enum value {} has no name and does not fit in a TOML integer.", static_cast<u64>(value)));
        return;
    }
    PlaceValue(value);
}

void TomlWriter::BeginStruct()
{
    if (HasError())
    {
        return;
    }

    // 루트 struct는 넘겨받은 테이블에 바로 씀
    if (!root_started)
    {
        root_started = true;
        open_containers.Push(OpenContainer{ .kind = EContainerKind::Struct, .node = &root });
        return;
    }

    if (toml::node* const table = PlaceValue(toml::table{}))
    {
        open_containers.Push(OpenContainer{ .kind = EContainerKind::Struct, .node = table });
    }
}

void TomlWriter::Field(StringView name)
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Struct)
    {
        SetError(String::Format("TomlWriter: field '{}' is outside a struct.", name));
        return;
    }
    if (top->pending_key)
    {
        SetError(String::Format("TomlWriter: field '{}' has no value.", *top->pending_key));
        return;
    }
    top->pending_key = String(name);
}

void TomlWriter::EndStruct()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Struct)
    {
        SetError("TomlWriter: EndStruct does not match an open struct.");
        return;
    }
    if (top->pending_key)
    {
        SetError(String::Format("TomlWriter: field '{}' has no value.", *top->pending_key));
        return;
    }
    open_containers.Pop();
}

void TomlWriter::BeginSeq([[maybe_unused]] u64 count, ESeqOrder order)
{
    if (toml::node* const array = PlaceValue(toml::array{}))
    {
        // 순서 없는 시퀀스는 같은 값이면 같은 텍스트가 나오도록 EndSeq에서 정렬
        const EContainerKind kind = order == ESeqOrder::Unordered ? EContainerKind::UnorderedSeq : EContainerKind::Seq;
        open_containers.Push(OpenContainer{ .kind = kind, .node = array });
    }
}

void TomlWriter::EndSeq()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || (top->kind != EContainerKind::Seq && top->kind != EContainerKind::UnorderedSeq))
    {
        SetError("TomlWriter: EndSeq does not match an open sequence.");
        return;
    }
    if (top->kind == EContainerKind::UnorderedSeq)
    {
        SortArray(*top->node->as_array());
    }
    open_containers.Pop();
}

void TomlWriter::BeginMap([[maybe_unused]] u64 count)
{
    // 테이블로 쓸 수 있는지는 key를 모두 봐야 알 수 있으므로, 넣을 자리만 확인하고 EndMap까지 엔트리를 모아 둠
    if (CanPlaceValue())
    {
        open_containers.Push(OpenContainer{ .kind = EContainerKind::Map });
    }
}

void TomlWriter::BeginMapEntry()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Map)
    {
        SetError("TomlWriter: BeginMapEntry is outside a map.");
        return;
    }

    // key와 value를 차례로 받을 [key, value] 배열
    top->map_entries.push_back(toml::array{});
    open_containers.Push(OpenContainer{ .kind = EContainerKind::MapEntry, .node = &top->map_entries.back() });
}

void TomlWriter::EndMapEntry()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::MapEntry)
    {
        SetError("TomlWriter: EndMapEntry does not match an open map entry.");
        return;
    }
    if (top->node->as_array()->size() != 2)
    {
        SetError("TomlWriter: a map entry needs exactly one key and one value.");
        return;
    }
    open_containers.Pop();
}

void TomlWriter::EndMap()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Map)
    {
        SetError("TomlWriter: EndMap does not match an open map.");
        return;
    }
    toml::array entries = std::move(top->map_entries);
    open_containers.Pop();

    // key가 모두 문자열이면 테이블 (빈 맵 포함). 테이블은 키 순서로 저장되므로 따로 정렬하지 않음
    const bool has_only_string_keys = std::all_of(entries.cbegin(), entries.cend(), [](const toml::node& entry)
    {
        return entry.as_array()->front().is_string();
    });
    if (has_only_string_keys)
    {
        toml::table table;
        for (toml::node& entry : entries)
        {
            toml::array& pair = *entry.as_array();
            const std::string& key = pair.front().as_string()->get();

            // 서로 다른 key가 같은 문자열로 쓰이면 한쪽이 사라지므로 오류로 처리
            if (!table.insert(key, std::move(pair.back())).second)
            {
                SetError(String::Format("TomlWriter: map key '{}' is written twice.", key));
                return;
            }
        }
        PlaceValue(std::move(table));
        return;
    }

    // 아니면 key 순서(key가 같으면 value 순서)로 정렬한 [key, value] 쌍 배열
    SortArray(entries);
    PlaceValue(std::move(entries));
}

void TomlWriter::Present(bool has_value)
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    const bool is_field_value = top && top->kind == EContainerKind::Struct && top->pending_key;
    if (has_value)
    {
        // 이어서 쓰는 내부 값이 이 자리에 들어감
        if (is_field_value)
        {
            top->pending_key_in_some = true;
        }
        return;
    }

    // None은 struct 필드의 키를 생략해서만 쓸 수 있음. 시퀀스 원소, 맵의 key와 value를 생략하면 그 자리가 사라짐
    if (!is_field_value)
    {
        SetError("TomlWriter: None can only be written as a struct field, by omitting its key.");
        return;
    }

    // Optional<Optional<T>>의 Some(None)은 키를 생략하면 바깥 None으로 읽힘
    if (top->pending_key_in_some)
    {
        SetError("TomlWriter: None inside another Optional cannot be written because the omitted key reads back as the outer None.");
        return;
    }
    top->pending_key.Reset();
}

bool TomlWriter::CanPlaceValue()
{
    if (HasError())
    {
        return false;
    }

    const auto top = open_containers.Peek();
    if (!top)
    {
        SetError("TomlWriter: the root value must be a single struct because a TOML document is a table.");
        return false;
    }
    if (top->kind == EContainerKind::Struct && !top->pending_key)
    {
        SetError("TomlWriter: a value inside a struct needs a Field name first.");
        return false;
    }
    if (top->kind == EContainerKind::Map)
    {
        SetError("TomlWriter: a value inside a map needs BeginMapEntry first.");
        return false;
    }
    return true;
}

template <typename Value>
toml::node* TomlWriter::PlaceValue(Value&& value)
{
    if (!CanPlaceValue())
    {
        return nullptr;
    }

    // struct면 Field가 정한 키로 넣음
    const auto top = open_containers.Peek();
    if (top->kind == EContainerKind::Struct)
    {
        const auto position = top->node->as_table()->insert_or_assign(ToStdStringView(*top->pending_key), std::forward<Value>(value)).first;
        top->pending_key.Reset();
        top->pending_key_in_some = false;
        return &position->second;
    }

    // 시퀀스와 맵 엔트리는 배열 끝에 넣음
    toml::array* const array = top->node->as_array();
    array->push_back(std::forward<Value>(value));
    return &array->back();
}


// TomlReader
TomlReader::TomlReader(const toml::table& in_root)
    : root(in_root)
{
}

ArrayView<const String> TomlReader::GetWarnings() const
{
    return warnings;
}

bool TomlReader::IsTextFormat() const
{
    return true;
}

void TomlReader::Int(i64& value, EIntWidth width, bool is_signed)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (const auto number = ReadInteger(*node, width, is_signed))
    {
        value = *number;
    }
}

void TomlReader::Float(f64& value, EFloatWidth width)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    // 사람이 17처럼 정수로 적은 실수도 받음
    f64 number = 0.0;
    if (const toml::value<f64>* const floating = node->as_floating_point())
    {
        number = floating->get();
    }
    else if (const toml::value<i64>* const integer = node->as_integer())
    {
        number = static_cast<f64>(integer->get());
    }
    else
    {
        SetError(String::Format("TomlReader: expected a float, got {}.", NodeKindName(*node)));
        return;
    }

    if (width == EFloatWidth::Bits32)
    {
        // f32로 반올림해 무한대가 되는 값만 오류 (FLT_MAX보다 조금 큰 3.4028235e+38은 반올림하면 FLT_MAX)
        const f32 narrowed = static_cast<f32>(number);
        if (std::isinf(narrowed) && std::isfinite(number))
        {
            SetError(String::Format("TomlReader: {} is out of range for f32.", number));
            return;
        }
        value = narrowed;
        return;
    }
    value = number;
}

void TomlReader::Bool(bool& value)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    const toml::value<bool>* const boolean = node->as_boolean();
    if (boolean == nullptr)
    {
        SetError(String::Format("TomlReader: expected a boolean, got {}.", NodeKindName(*node)));
        return;
    }
    value = boolean->get();
}

void TomlReader::Str(String& value)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    const toml::value<std::string>* const text = node->as_string();
    if (text == nullptr)
    {
        SetError(String::Format("TomlReader: expected a string, got {}.", NodeKindName(*node)));
        return;
    }
    value = str::ToString(text->get());
}

void TomlReader::Bytes(void* data, u64 size)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    const toml::value<std::string>* const text = node->as_string();
    if (text == nullptr)
    {
        SetError(String::Format("TomlReader: expected a base64 string, got {}.", NodeKindName(*node)));
        return;
    }

    const auto bytes = base64::Decode(std::string_view{ text->get() });
    if (!bytes)
    {
        SetError("TomlReader: invalid base64 string.");
        return;
    }
    if (bytes->Len() != size)
    {
        SetError(String::Format("TomlReader: expected {} bytes, got {} bytes of base64 data.", size, bytes->Len()));
        return;
    }
    std::ranges::copy(*bytes, static_cast<u8*>(data));
}

void TomlReader::Enum(i64& value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    // 이름으로 적힌 값
    if (const toml::value<std::string>* const text = node->as_string())
    {
        const StringView name = std::string_view{ text->get() };
        const auto* const entry = std::ranges::find(entries, name, &EnumEntry::name);
        if (entry == entries.end())
        {
            SetError(String::Format("TomlReader: '{}' is not a name of this enum.", name));
            return;
        }
        value = entry->value;
        return;
    }

    // 정수로 적힌 값 (이름이 없는 값, 레거시 출력)
    if (const auto number = ReadInteger(*node, width, is_signed))
    {
        value = *number;
    }
}

void TomlReader::BeginStruct()
{
    if (HasError())
    {
        return;
    }

    // 루트 struct는 넘겨받은 테이블에서 바로 읽음
    if (!root_started)
    {
        root_started = true;
        open_containers.Push(OpenContainer{ .kind = EContainerKind::Struct, .node = &root });
        return;
    }

    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }
    if (!node->is_table())
    {
        SetError(String::Format("TomlReader: expected a table, got {}.", NodeKindName(*node)));
        return;
    }
    open_containers.Push(OpenContainer{ .kind = EContainerKind::Struct, .node = node, .path = PathOfTakenValue() });
}

bool TomlReader::Field(StringView name)
{
    if (HasError())
    {
        return false;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Struct)
    {
        SetError(String::Format("TomlReader: field '{}' is outside a struct.", name));
        return false;
    }

    // 타입이 물어본 필드 이름을 기억해 두고, EndStruct에서 타입에 없는 키를 찾음
    top->known_keys.Push(name);

    // 없는 필드는 false (호출자가 현재 값을 유지)
    top->field_value = top->node->as_table()->get(ToStdStringView(name));
    return top->field_value != nullptr;
}

void TomlReader::EndStruct()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Struct)
    {
        SetError("TomlReader: EndStruct does not match an open struct.");
        return;
    }

    // 타입에 없는 키는 경고로 남기고 읽기는 계속함 (필드 이름을 바꿨거나 오타를 냈을 때 알아차리게)
    for (const auto& entry : *top->node->as_table())
    {
        const StringView key = entry.first.str();
        const bool is_known = std::ranges::any_of(top->known_keys, [&](const String& known_key)
        {
            return known_key == key;
        });
        if (!is_known)
        {
            warnings.Push(String::Format("TomlReader: unknown key '{}' is ignored.", JoinPath(top->path, key)));
        }
    }
    open_containers.Pop();
}

void TomlReader::BeginSeq(u64& count)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    const toml::array* const array = node->as_array();
    if (array == nullptr)
    {
        SetError(String::Format("TomlReader: expected an array, got {}.", NodeKindName(*node)));
        return;
    }
    count = array->size();
    open_containers.Push(OpenContainer{ .kind = EContainerKind::Seq, .node = array, .path = PathOfTakenValue() });
}

void TomlReader::EndSeq()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Seq)
    {
        SetError("TomlReader: EndSeq does not match an open sequence.");
        return;
    }
    open_containers.Pop();
}

void TomlReader::BeginMap(u64& count)
{
    const toml::node* const node = TakeValue();
    if (node == nullptr)
    {
        return;
    }

    // key가 모두 문자열이면 테이블, 아니면 [key, value] 쌍 배열로 쓰여 있음 (빈 맵은 둘 다 받음)
    if (const toml::table* const table = node->as_table())
    {
        count = table->size();
        open_containers.Push(OpenContainer{
            .kind = EContainerKind::Map,
            .node = table,
            .path = PathOfTakenValue(),
            .next_entry = table->cbegin(),
        });
        return;
    }
    if (const toml::array* const array = node->as_array())
    {
        count = array->size();
        open_containers.Push(OpenContainer{ .kind = EContainerKind::Map, .node = array, .path = PathOfTakenValue() });
        return;
    }
    SetError(String::Format("TomlReader: expected a table or an array, got {}.", NodeKindName(*node)));
}

void TomlReader::BeginMapEntry()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Map)
    {
        SetError("TomlReader: BeginMapEntry is outside a map.");
        return;
    }

    // 테이블 맵의 엔트리는 키를 문자열 노드로 만들어, key 자리에서 문자열이나 enum 이름으로 읽히게 함
    if (const toml::table* const table = top->node->as_table())
    {
        if (top->next_entry == table->cend())
        {
            SetError(String::Format("TomlReader: read past the end of a table of {} entries.", table->size()));
            return;
        }
        const std::string_view key = top->next_entry->first.str();
        const toml::node& value = top->next_entry->second;
        ++top->next_entry;
        open_containers.Push(OpenContainer{
            .kind = EContainerKind::MapEntry,
            .node = &value,
            .path = JoinPath(top->path, key),
            .table_key = toml::value<std::string>{ std::string{ key } },
        });
        return;
    }

    // 쌍 배열 맵의 엔트리는 원소 하나가 [key, value] 배열
    const toml::array& pairs = *top->node->as_array();
    if (top->next_index >= pairs.size())
    {
        SetError(String::Format("TomlReader: read past the end of an array of length {}.", pairs.size()));
        return;
    }
    const usize index = top->next_index++;
    const toml::array* const pair = pairs[index].as_array();
    if (pair == nullptr)
    {
        SetError(String::Format("TomlReader: expected a [key, value] array, got {}.", NodeKindName(pairs[index])));
        return;
    }
    if (pair->size() != 2)
    {
        SetError(String::Format("TomlReader: expected a [key, value] array, got an array of length {}.", pair->size()));
        return;
    }
    open_containers.Push(OpenContainer{ .kind = EContainerKind::MapEntry, .node = pair, .path = String::Format("{}[{}]", top->path, index) });
}

void TomlReader::EndMapEntry()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::MapEntry)
    {
        SetError("TomlReader: EndMapEntry does not match an open map entry.");
        return;
    }
    open_containers.Pop();
}

void TomlReader::EndMap()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || top->kind != EContainerKind::Map)
    {
        SetError("TomlReader: EndMap does not match an open map.");
        return;
    }
    open_containers.Pop();
}

void TomlReader::Present(bool& has_value)
{
    // None은 struct 필드의 키를 생략해서만 쓰므로 읽을 값이 있는 자리는 항상 Some (없는 필드는 Field가 false)
    if (!HasError())
    {
        has_value = true;
    }
}

const toml::node* TomlReader::TakeValue()
{
    if (HasError())
    {
        return nullptr;
    }

    const auto top = open_containers.Peek();
    if (!top)
    {
        SetError("TomlReader: the root value must be a single struct because a TOML document is a table.");
        return nullptr;
    }

    switch (top->kind)
    {
    case EContainerKind::Struct:
        // Field가 찾아 둔 값
        if (top->field_value == nullptr)
        {
            SetError("TomlReader: a value inside a struct needs a Field name first.");
            return nullptr;
        }
        return std::exchange(top->field_value, nullptr);

    case EContainerKind::Seq:
    {
        // 다음 원소
        const toml::array& array = *top->node->as_array();
        if (top->next_index >= array.size())
        {
            SetError(String::Format("TomlReader: read past the end of an array of length {}.", array.size()));
            return nullptr;
        }
        return array.get(top->next_index++);
    }

    case EContainerKind::Map:
        SetError("TomlReader: a value inside a map needs BeginMapEntry first.");
        return nullptr;

    case EContainerKind::MapEntry:
    {
        // key 다음에 value
        if (top->next_index >= 2)
        {
            SetError("TomlReader: a map entry has only a key and a value.");
            return nullptr;
        }
        const usize slot = top->next_index++;
        if (top->table_key)
        {
            return slot == 0 ? &*top->table_key : top->node;
        }
        return top->node->as_array()->get(slot);
    }
    }
    SE_UNREACHABLE();
}

Optional<i64> TomlReader::ReadInteger(const toml::node& node, EIntWidth width, bool is_signed)
{
    if (const toml::value<i64>* const integer = node.as_integer())
    {
        const i64 number = integer->get();
        if (!FitsInWidth(number, width, is_signed))
        {
            SetError(String::Format("TomlReader: {} is out of range for {}.", number, IntTypeName(width, is_signed)));
            return NullOpt;
        }
        return number;
    }

    // i64를 넘는 u64는 10진 문자열로 저장됨
    const toml::value<std::string>* const text = node.as_string();
    if (text != nullptr && width == EIntWidth::Bits64 && !is_signed)
    {
        const std::string& digits = text->get();
        u64 number = 0;
        const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), number);
        if (error != std::errc{} || end != digits.data() + digits.size())
        {
            SetError(String::Format("TomlReader: '{}' is not a valid u64 number.", digits));
            return NullOpt;
        }
        return static_cast<i64>(number);
    }

    SetError(String::Format("TomlReader: expected an integer, got {}.", NodeKindName(node)));
    return NullOpt;
}

String TomlReader::PathOfTakenValue() const
{
    const auto parent = open_containers.Peek();
    switch (parent->kind)
    {
    case EContainerKind::Struct:
        // 마지막으로 물어본 필드 이름이 방금 꺼낸 값의 키
        return JoinPath(parent->path, *parent->known_keys.Back());

    case EContainerKind::Seq:
        // TakeValue가 이미 다음 번호로 넘어갔으므로 하나 앞이 방금 꺼낸 원소
        return String::Format("{}[{}]", parent->path, parent->next_index - 1);

    case EContainerKind::MapEntry:
        // 테이블 맵은 key와 value가 엔트리의 위치("scores.alice"), 쌍 배열 맵은 [key, value] 배열 안의 번호("points[0][1]")
        if (parent->table_key)
        {
            return parent->path;
        }
        return String::Format("{}[{}]", parent->path, parent->next_index - 1);

    case EContainerKind::Map:
        // TakeValue가 맵에서는 값을 꺼내지 않음
        break;
    }
    SE_UNREACHABLE();
}
} // namespace se
