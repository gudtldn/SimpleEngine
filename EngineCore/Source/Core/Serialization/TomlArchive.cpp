#include "Core/Serialization/TomlArchive.h"

#include <charconv>
#include <string_view>

#include "Core/Reflection/TypeId.h"
#include "Core/Types/Guid.h"
#include "Core/Types/StringName.h"
#include "Utility/Debug.h"
#include "Utility/StringUtils.h"


namespace
{
std::u8string_view ToU8StringView(se::StringView sv)
{
    return { reinterpret_cast<const char8_t*>(sv.Data()), sv.ByteLen() };
}

template <typename T>
void FromChars(const se::String& str, T& value)
{
    const auto result = std::from_chars(str.Data(), str.Data() + str.ByteLen(), value);
    if (result.ec != std::errc{})
    {
        if (result.ec == std::errc::invalid_argument)
        {
            SE_ENSURE(false, "FromChars: Invalid argument when parsing '{}' as numeric value", str);
        }
        else if (result.ec == std::errc::result_out_of_range)
        {
            SE_ENSURE(false, "FromChars: Result out of range when parsing '{}' as numeric value", str);
        }
        value = T{};
    }
}
}

namespace se
{
// TomlArchive (공통 기반)
TomlArchive::Context& TomlArchive::GetCurrentContext()
{
    SE_ASSERT(!context_stack.IsEmpty(), "Context stack underflow!");
    return context_stack.Top().Value();
}


// TomlReader
TomlReader::TomlReader(const toml::table& root)
    : TomlArchive(EArchiveMode::Load)
{
    context_stack.Push({
        .node = const_cast<toml::table*>(&root),
        .mode = EMode::None,
    });
}

void TomlReader::BeginObject()
{
    const Context& ctx = GetCurrentContext();
    if (!ctx.IsArray() && pending_key.IsEmpty())
    {
        context_stack.Push(ctx);
        return;
    }

    toml::node* sub_node = GetCurrentNode();
    if (SE_ENSURE(sub_node && sub_node->is_table(), "TomlReader::BeginObject - Expected a table node. (pending_key: '{}')", pending_key))
    {
        context_stack.Push({
            .node = sub_node,
            .mode = EMode::None,
        });
    }
    else
    {
        context_stack.Push({
            .node = nullptr,
            .mode = EMode::None,
        });
    }
}

void TomlReader::EndObject()
{
    context_stack.Pop();
}

void TomlReader::BeginArray(uint64& count)
{
    toml::node* sub_node = GetCurrentNode();
    if (SE_ENSURE(sub_node && sub_node->is_array(), "TomlReader::BeginArray - Expected an array node. (pending_key: '{}')", pending_key))
    {
        toml::array* arr = sub_node->as_array();
        count = arr->size();
        context_stack.Push({
            .node = arr,
            .mode = EMode::ArrayMode,
            .array_idx = 0,
        });
    }
    else
    {
        count = 0;
        context_stack.Push({
            .node = nullptr,
            .mode = EMode::ArrayMode,
            .array_idx = 0,
        });
    }
}

void TomlReader::EndArray()
{
    context_stack.Pop();
}

void TomlReader::BeginMap(uint64& count)
{
    toml::node* sub_node = GetCurrentNode();
    if (SE_ENSURE(sub_node && sub_node->is_table(), "TomlReader::BeginMap - Expected a table node. (pending_key: '{}')", pending_key))
    {
        toml::table* tbl = sub_node->as_table();
        count = tbl->size();
        context_stack.Push({
            .node = tbl,
            .mode = EMode::MapMode,
            .map_it = tbl->begin(),
            .map_end = tbl->end(),
        });
    }
    else
    {
        count = 0;
        static toml::table empty_table;
        context_stack.Push({
            .node = nullptr,
            .mode = EMode::MapMode,
            .map_it = empty_table.begin(),
            .map_end = empty_table.end(),
        });
    }
}

void TomlReader::EndMap()
{
    context_stack.Pop();
}

void TomlReader::BeginMapKey()
{
    Context& ctx = GetCurrentContext();
    if (!SE_ENSURE(ctx.node && ctx.IsMap(), "TomlReader::BeginMapKey - Invalid context. (node: {}, IsMap: {})", (void*)ctx.node, ctx.IsMap()))
    {
        return;
    }

    SE_ENSURE(ctx.map_it != ctx.map_end, "TomlReader::BeginMapKey - Map iterator already at end.");
    if (ctx.map_it != ctx.map_end)
    {
        current_map_key = StringUtils::ToString(ctx.map_it->first);
    }

    reading_map_key = true;
}

void TomlReader::EndMapKey()
{
    reading_map_key = false;
}

void TomlReader::BeginMapValue()
{
    pending_key = current_map_key;
}

void TomlReader::EndMapValue()
{
    // iterator 전진
    Context& ctx = GetCurrentContext();
    if (ctx.IsMap() && ctx.map_it != ctx.map_end)
    {
        ++ctx.map_it;
    }
}

// Raw 바이트 (텍스트에서는 미지원)
void TomlReader::SerializeBytes([[maybe_unused]] void* data, [[maybe_unused]] uint64 size)
{
    // TODO: Base64 디코딩 지원
}

void TomlReader::HintNextName(const char* name)
{
    pending_key = name;
}

void TomlReader::SerializeBool(bool& value)
{
    if (reading_map_key)
    {
        value = (current_map_key == "true");
        return;
    }
    ReadValue(value);
}

void TomlReader::SerializeInt8(int8& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<int8>(temp);
    }
}

void TomlReader::SerializeUInt8(uint8& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<uint8>(temp);
    }
}

void TomlReader::SerializeInt16(int16& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<int16>(temp);
    }
}

void TomlReader::SerializeUInt16(uint16& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<uint16>(temp);
    }
}

void TomlReader::SerializeInt32(int32& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<int32>(temp);
    }
}

void TomlReader::SerializeUInt32(uint32& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<uint32>(temp);
    }
}

void TomlReader::SerializeInt64(int64& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }
    ReadValue(value);
}

void TomlReader::SerializeUInt64(uint64& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<uint64>(temp);
    }
}

void TomlReader::SerializeFloat(float& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    double temp = 0.0;
    if (ReadValue(temp))
    {
        value = static_cast<float>(temp);
    }
}

void TomlReader::SerializeDouble(double& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }
    ReadValue(value);
}

void TomlReader::SerializeString(String& value)
{
    if (reading_map_key)
    {
        value = current_map_key;
        return;
    }

    std::u8string_view sv;
    if (ReadValue(sv))
    {
        value = StringUtils::ToString(sv);
    }
}

void TomlReader::SerializeStringName(StringName& value)
{
    if (reading_map_key)
    {
        value = current_map_key;
        return;
    }

    std::u8string_view sv;
    if (ReadValue(sv))
    {
        value = StringUtils::ToString(sv);
    }
}

void TomlReader::SerializeGuid(Guid& value)
{
    if (reading_map_key)
    {
        value = Guid::FromString(current_map_key);
        return;
    }

    std::u8string_view sv;
    if (ReadValue(sv))
    {
        value = Guid::FromString(StringUtils::ToString(sv));
    }
}

void TomlReader::SerializeTypeId(TypeId& value)
{
    String type_name;
    SerializeString(type_name);

    value = TypeId::FromName(type_name);
    SE_ENSURE(value.IsValid(), "TomlReader::SerializeTypeId - Failed to resolve TypeId from name: '{}'. The class might be deleted or renamed.", type_name);
}

toml::node* TomlReader::GetCurrentNode()
{
    Context& ctx = GetCurrentContext();
    if (!ctx.node)
    {
        return nullptr;
    }

    if (ctx.IsArray())
    {
        toml::array* arr = ctx.node->as_array();
        if (ctx.array_idx < arr->size())
        {
            return arr->get(ctx.array_idx++);
        }
        return nullptr;
    }

    if (!pending_key.IsEmpty())
    {
        toml::node* node = ctx.node->as_table()->get(pending_key);
        pending_key = "";
        return node;
    }
    return nullptr;
}


// TomlWriter
TomlWriter::TomlWriter(toml::table& root)
    : TomlArchive(EArchiveMode::Save)
{
    context_stack.Push({
        .node = &root,
        .mode = EMode::None,
    });
}

void TomlWriter::BeginObject()
{
    const Context& ctx = GetCurrentContext();
    if (!ctx.IsArray() && pending_key.IsEmpty())
    {
        context_stack.Push(ctx);
        return;
    }

    context_stack.Push({
        .node = InsertNewNode<toml::table>(toml::table{}),
        .mode = EMode::None,
    });
}

void TomlWriter::EndObject()
{
    context_stack.Pop();
}

void TomlWriter::BeginArray([[maybe_unused]] uint64& count)
{
    toml::array* arr = InsertNewNode<toml::array>(toml::array{});
    SE_ENSURE(arr, "TomlWriter::BeginArray - Failed to insert new array node.");
    context_stack.Push({
        .node = arr,
        .mode = EMode::ArrayMode,
        .array_idx = 0,
    });
}

void TomlWriter::EndArray()
{
    context_stack.Pop();
}

void TomlWriter::BeginMap([[maybe_unused]] uint64& count)
{
    // Map을 TOML table로 표현
    toml::table* tbl = InsertNewNode<toml::table>(toml::table{});
    SE_ENSURE(tbl, "TomlWriter::BeginMap - Failed to insert new table node.");
    context_stack.Push({
        .node = tbl,
        .mode = EMode::MapMode,
    });
}

void TomlWriter::EndMap()
{
    context_stack.Pop();
}

void TomlWriter::BeginMapKey()
{
    // Key 직렬화 시작 - 캡처 모드 ON
    current_map_key.Clear();
    capturing_map_key = true;
}

void TomlWriter::EndMapKey()
{
    // 캡처 모드 OFF, 캡처된 key를 pending_key로 설정
    SE_ENSURE(!current_map_key.IsEmpty(), "TomlWriter::EndMapKey - Captured map key is empty.");
    capturing_map_key = false;
    pending_key = current_map_key;
}

void TomlWriter::BeginMapValue() {}
void TomlWriter::EndMapValue() {}

// Raw 바이트 (텍스트에서는 미지원)
void TomlWriter::SerializeBytes([[maybe_unused]] void* data, [[maybe_unused]] uint64 size)
{
    // TODO: Base64 인코딩 지원
}

void TomlWriter::HintNextName(const char* name)
{
    pending_key = name;
}

// Map key 캡처 모드에서는 값을 TOML에 쓰지 않고 current_map_key에 문자열로 저장
// 이후 table에 쓸 때 Key로 사용
void TomlWriter::SerializeBool(bool& value)
{
    if (capturing_map_key)
    {
        current_map_key = value ? "true" : "false";
        return;
    }
    WriteValue(value);
}

void TomlWriter::SerializeInt8(int8& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeUInt8(uint8& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeInt16(int16& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeUInt16(uint16& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeInt32(int32& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeUInt32(uint32& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeInt64(int64& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(value);
}

void TomlWriter::SerializeUInt64(uint64& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<int64>(value));
}

void TomlWriter::SerializeFloat(float& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<double>(value));
}

void TomlWriter::SerializeDouble(double& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(value);
}

void TomlWriter::SerializeString(String& value)
{
    if (capturing_map_key)
    {
        current_map_key = value;
        return;
    }
    WriteValue(ToU8StringView(value));
}

void TomlWriter::SerializeStringName(StringName& value)
{
    if (capturing_map_key)
    {
        current_map_key = value.ToString();
        return;
    }
    WriteValue(ToU8StringView(value.CStr()));
}

void TomlWriter::SerializeGuid(Guid& value)
{
    if (capturing_map_key)
    {
        current_map_key = value.ToString();
        return;
    }
    WriteValue(ToU8StringView(value.ToString()));
}

void TomlWriter::SerializeTypeId(TypeId& value)
{
    String type_name;
    if (SE_ENSURE(value.IsValid(), "Attempting to save invalid TypeId via Text!"))
    {
        type_name = value.GetName();
    }
    SerializeString(type_name);
}
} // namespace se
