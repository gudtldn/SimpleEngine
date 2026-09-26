#include "SimpleEngine/Core/Serialization/TomlArchive.h"

#include "SimpleEngine/Utility/StringUtils.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iterator>
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

void TomlWriter::Bytes([[maybe_unused]] const void* data, [[maybe_unused]] u64 size)
{
    SetError("TomlWriter: bytes are not supported yet.");
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
        open_containers.Push(OpenContainer{ .node = &root });
        return;
    }

    if (toml::node* const table = PlaceValue(toml::table{}))
    {
        open_containers.Push(OpenContainer{ .node = table });
    }
}

void TomlWriter::Field(StringView name)
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || !top->node->is_table())
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
    if (!top || !top->node->is_table())
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
    if (order == ESeqOrder::Unordered)
    {
        SetError("TomlWriter: unordered sequences are not supported yet.");
        return;
    }

    if (toml::node* const array = PlaceValue(toml::array{}))
    {
        open_containers.Push(OpenContainer{ .node = array });
    }
}

void TomlWriter::EndSeq()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || !top->node->is_array())
    {
        SetError("TomlWriter: EndSeq does not match an open sequence.");
        return;
    }
    open_containers.Pop();
}

void TomlWriter::BeginMap([[maybe_unused]] u64 count)
{
    SetError("TomlWriter: maps are not supported yet.");
}

void TomlWriter::BeginMapEntry()
{
    SetError("TomlWriter: maps are not supported yet.");
}

void TomlWriter::EndMapEntry()
{
    SetError("TomlWriter: maps are not supported yet.");
}

void TomlWriter::EndMap()
{
    SetError("TomlWriter: maps are not supported yet.");
}

void TomlWriter::Present([[maybe_unused]] bool has_value)
{
    SetError("TomlWriter: optional values are not supported yet.");
}

template <typename Value>
toml::node* TomlWriter::PlaceValue(Value&& value)
{
    if (HasError())
    {
        return nullptr;
    }

    const auto top = open_containers.Peek();
    if (!top)
    {
        SetError("TomlWriter: the root value must be a single struct because a TOML document is a table.");
        return nullptr;
    }

    // 배열이면 끝에 넣음
    if (toml::array* const array = top->node->as_array())
    {
        array->push_back(std::forward<Value>(value));
        return &array->back();
    }

    // 테이블이면 Field가 정한 키로 넣음
    if (!top->pending_key)
    {
        SetError("TomlWriter: a value inside a struct needs a Field name first.");
        return nullptr;
    }
    const auto position = top->node->as_table()->insert_or_assign(ToStdStringView(*top->pending_key), std::forward<Value>(value)).first;
    top->pending_key.Reset();
    return &position->second;
}


// TomlReader
TomlReader::TomlReader(const toml::table& in_root)
    : root(in_root)
{
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

void TomlReader::Bytes([[maybe_unused]] void* data, [[maybe_unused]] u64 size)
{
    SetError("TomlReader: bytes are not supported yet.");
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
        open_containers.Push(OpenContainer{ .node = &root });
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
    open_containers.Push(OpenContainer{ .node = node });
}

bool TomlReader::Field(StringView name)
{
    if (HasError())
    {
        return false;
    }

    const auto top = open_containers.Peek();
    if (!top || !top->node->is_table())
    {
        SetError(String::Format("TomlReader: field '{}' is outside a struct.", name));
        return false;
    }

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
    if (!top || !top->node->is_table())
    {
        SetError("TomlReader: EndStruct does not match an open struct.");
        return;
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
    open_containers.Push(OpenContainer{ .node = array });
}

void TomlReader::EndSeq()
{
    if (HasError())
    {
        return;
    }

    const auto top = open_containers.Peek();
    if (!top || !top->node->is_array())
    {
        SetError("TomlReader: EndSeq does not match an open sequence.");
        return;
    }
    open_containers.Pop();
}

void TomlReader::BeginMap([[maybe_unused]] u64& count)
{
    SetError("TomlReader: maps are not supported yet.");
}

void TomlReader::BeginMapEntry()
{
    SetError("TomlReader: maps are not supported yet.");
}

void TomlReader::EndMapEntry()
{
    SetError("TomlReader: maps are not supported yet.");
}

void TomlReader::EndMap()
{
    SetError("TomlReader: maps are not supported yet.");
}

void TomlReader::Present([[maybe_unused]] bool& has_value)
{
    SetError("TomlReader: optional values are not supported yet.");
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

    // 배열이면 다음 원소
    if (const toml::array* const array = top->node->as_array())
    {
        if (top->next_index >= array->size())
        {
            SetError(String::Format("TomlReader: read past the end of an array of length {}.", array->size()));
            return nullptr;
        }
        return array->get(top->next_index++);
    }

    // 테이블이면 Field가 찾아 둔 값
    if (top->field_value == nullptr)
    {
        SetError("TomlReader: a value inside a struct needs a Field name first.");
        return nullptr;
    }
    return std::exchange(top->field_value, nullptr);
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
} // namespace se
