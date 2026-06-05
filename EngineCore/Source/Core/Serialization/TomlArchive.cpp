#include "SimpleEngine/Core/Serialization/TomlArchive.h"

#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Utility/Debug.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include <charconv>
#include <string_view>


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
    if (static_cast<int>(result.ec) != 0)
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
} // namespace

namespace se
{
// TomlArchive (공통 기반)
TomlArchive::Context& TomlArchive::GetCurrentContext()
{
    SE_ASSERT(!context_stack.IsEmpty(), "Context stack underflow!");
    return context_stack.Back().Value();
}

bool TomlArchive::IsRootNode(const toml::node* node) const
{
    SE_ASSERT(!context_stack.IsEmpty(), "Context stack underflow!");
    return node == context_stack.Front().Value().node;
}


// TomlReader
TomlReader::TomlReader(const toml::table& root)
    : TomlArchive(EArchiveMode::Load)
{
    context_stack.Push({
        .node = const_cast<toml::table*>(&root),
        .mode = EContextMode::Object,
    });
}

void TomlReader::BeginObject()
{
    Context ctx = GetCurrentContext();
    if (!ctx.IsArray() && pending_key.IsEmpty())
    {
        SE_ASSERT(IsRootNode(ctx.node), "TomlReader::BeginObject - Missing key! This is not the Root node.");
        context_stack.Push(std::move(ctx));
        return;
    }

    toml::node* sub_node = GetCurrentNode();
    if (SE_ENSURE(sub_node && sub_node->is_table(), "TomlReader::BeginObject - Expected a table node. (pending_key: '{}')", pending_key))
    {
        context_stack.Push({
            .node = sub_node,
            .mode = EContextMode::Object,
        });
    }
    else
    {
        SetError(String::Format("TomlReader: Expected a table node for key '{}'.", pending_key));
        context_stack.Push({
            .node = nullptr,
            .mode = EContextMode::Object,
        });
    }
}

void TomlReader::EndObject()
{
    context_stack.Pop();
}

void TomlReader::BeginArray(u64& count)
{
    toml::node* sub_node = GetCurrentNode();
    if (SE_ENSURE(sub_node && sub_node->is_array(), "TomlReader::BeginArray - Expected an array node. (pending_key: '{}')", pending_key))
    {
        toml::array* arr = sub_node->as_array();
        count = arr->size();
        context_stack.Push({
            .node = arr,
            .mode = EContextMode::Array,
            .array_idx = 0,
        });
    }
    else
    {
        SetError(String::Format("TomlReader: Expected an array node for key '{}'.", pending_key));
        count = 0;
        context_stack.Push({
            .node = nullptr,
            .mode = EContextMode::Array,
            .array_idx = 0,
        });
    }
}

void TomlReader::EndArray()
{
    context_stack.Pop();
}

void TomlReader::BeginMap(u64& count)
{
    const Context& ctx = GetCurrentContext();

    // Array가 아닌데, Key가 지정되지 않은 경우
    if (!ctx.IsArray() && pending_key.IsEmpty())
    {
        // Root 노드이거나, Archive::operator<<에 의해 방금 열린 Object일 경우,
        // 이 노드 자체를 Map으로 취급하여 현재 테이블에 그대로 데이터를 읽습니다.
        SE_ASSERT(ctx.IsObject(), "TomlReader::BeginMap - Invalid state! Cannot open anonymous map.");

        // Root 노드가 테이블 형태가 아닌 것은 파일 포맷 자체가 깨진 것이므로 방어
        if (SE_ENSURE(ctx.node && ctx.node->is_table(), "TomlReader::BeginMap - Root node is not a table."))
        {
            toml::table* tbl = ctx.node->as_table();
            count = tbl->size();
            context_stack.Push({
                .node = tbl,
                .mode = EContextMode::Map,
                .map_it = tbl->begin(),
                .map_end = tbl->end(),
            });
        }
        else
        {
            SetError("TomlReader: Root map node is not a table.");
            count = 0;
            static toml::table empty_table;
            context_stack.Push({
                .node = nullptr,
                .mode = EContextMode::Map,
                .map_it = empty_table.begin(),
                .map_end = empty_table.end(),
            });
        }
        return;
    }

    toml::node* sub_node = GetCurrentNode();
    if (SE_ENSURE(sub_node && sub_node->is_table(), "TomlReader::BeginMap - Expected a table node. (pending_key: '{}')", pending_key))
    {
        toml::table* tbl = sub_node->as_table();
        count = tbl->size();
        context_stack.Push({
            .node = tbl,
            .mode = EContextMode::Map,
            .map_it = tbl->begin(),
            .map_end = tbl->end(),
        });
    }
    else
    {
        SetError(String::Format("TomlReader: Expected a table node for map key '{}'.", pending_key));
        count = 0;
        static toml::table empty_table;
        context_stack.Push({
            .node = nullptr,
            .mode = EContextMode::Map,
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

    // Map 모드가 아닌데 BeginMapKey를 호출한 경우 Assert (직렬화 코드 작성 오류)
    SE_ASSERT(ctx.node && ctx.IsMap(), "TomlReader::BeginMapKey - Invalid context. (node: {}, IsMap: {})", static_cast<void*>(ctx.node), ctx.IsMap());

    // 모든 Iterator를 소모했으나, BeginMapKey를 호출한 경우 Assert (직렬화 코드 작성 오류)
    SE_ASSERT(ctx.map_it != ctx.map_end, "TomlReader::BeginMapKey - Map iterator already at end.");

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
void TomlReader::SerializeBytes([[maybe_unused]] void* data, [[maybe_unused]] u64 size)
{
    // TODO: Base64 디코딩 지원
}

void TomlReader::HintNextName(StringView name)
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

void TomlReader::SerializeInt8(i8& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<i8>(temp);
    }
}

void TomlReader::SerializeUInt8(u8& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<u8>(temp);
    }
}

void TomlReader::SerializeInt16(i16& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<i16>(temp);
    }
}

void TomlReader::SerializeUInt16(u16& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<u16>(temp);
    }
}

void TomlReader::SerializeInt32(i32& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<i32>(temp);
    }
}

void TomlReader::SerializeUInt32(u32& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<u32>(temp);
    }
}

void TomlReader::SerializeInt64(i64& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }
    ReadValue(value);
}

void TomlReader::SerializeUInt64(u64& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    i64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<u64>(temp);
    }
}

void TomlReader::SerializeFloat(f32& value)
{
    if (reading_map_key)
    {
        FromChars(current_map_key, value);
        return;
    }

    f64 temp = 0.0;
    if (ReadValue(temp))
    {
        value = static_cast<f32>(temp);
    }
}

void TomlReader::SerializeDouble(f64& value)
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
    if (!SE_ENSURE(value.IsValid(), "TomlReader::SerializeTypeId - Failed to resolve TypeId from name: '{}'. The class might be deleted or renamed.", type_name))
    {
        SetError(String::Format("TomlReader: Failed to resolve TypeId from name: '{}'.", type_name));
    }
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
        .mode = EContextMode::Object,
    });
}

void TomlWriter::BeginObject()
{
    Context ctx = GetCurrentContext();
    if (!ctx.IsArray() && pending_key.IsEmpty())
    {
        SE_ASSERT(IsRootNode(ctx.node), "TomlWriter::BeginObject - Missing key! Cannot open an anonymous object unless it is the Root node.");
        context_stack.Push(std::move(ctx));
        return;
    }

    toml::table* tbl = InsertNewNode<toml::table>(toml::table{});
    SE_ASSERT(tbl, "TomlWriter::BeginObject - Failed to insert new table node.");

    context_stack.Push({
        .node = tbl,
        .mode = EContextMode::Object,
    });
}

void TomlWriter::EndObject()
{
    context_stack.Pop();
}

void TomlWriter::BeginArray([[maybe_unused]] u64& count)
{
    toml::array* arr = InsertNewNode<toml::array>(toml::array{});
    SE_ASSERT(arr, "TomlWriter::BeginArray - Failed to insert new array node.");
    context_stack.Push({
        .node = arr,
        .mode = EContextMode::Array,
        .array_idx = 0,
    });
}

void TomlWriter::EndArray()
{
    context_stack.Pop();
}

void TomlWriter::BeginMap([[maybe_unused]] u64& count)
{
    const Context& ctx = GetCurrentContext();

    // ArrayMode가 아닌데, Key가 지정되지 않은 경우
    if (!ctx.IsArray() && pending_key.IsEmpty())
    {
        // Root 노드이거나, Archive::operator<<에 의해 방금 열린 Object일 경우,
        // 이 노드 자체를 Map으로 취급하여 현재 테이블에 그대로 데이터를 씁니다.
        SE_ASSERT(ctx.IsObject(), "TomlWriter::BeginMap - Invalid state! Cannot open anonymous map.");

        context_stack.Push({
            .node = ctx.node,
            .mode = EContextMode::Map,
        });
        return;
    }

    // Map을 TOML table로 표현
    toml::table* tbl = InsertNewNode<toml::table>(toml::table{});
    SE_ASSERT(tbl, "TomlWriter::BeginMap - Failed to insert new table node.");

    context_stack.Push({
        .node = tbl,
        .mode = EContextMode::Map,
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
    // Key 캡처 모드를 켰으나, 아무 값도 쓰지 않은 경우 Assert
    SE_ASSERT(!current_map_key.IsEmpty(), "TomlWriter::EndMapKey - Captured map key is empty.");

    // 캡처 모드 OFF, 캡처된 key를 pending_key로 설정
    capturing_map_key = false;
    pending_key = current_map_key;
}

void TomlWriter::BeginMapValue() {}
void TomlWriter::EndMapValue() {}

// Raw 바이트 (텍스트에서는 미지원)
void TomlWriter::SerializeBytes([[maybe_unused]] void* data, [[maybe_unused]] u64 size)
{
    // TODO: Base64 인코딩 지원
}

void TomlWriter::HintNextName(StringView name)
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

void TomlWriter::SerializeInt8(i8& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeUInt8(u8& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeInt16(i16& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeUInt16(u16& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeInt32(i32& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeUInt32(u32& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeInt64(i64& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(value);
}

void TomlWriter::SerializeUInt64(u64& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<i64>(value));
}

void TomlWriter::SerializeFloat(f32& value)
{
    if (capturing_map_key)
    {
        current_map_key = String::Format("{}", value);
        return;
    }
    WriteValue(static_cast<f64>(value));
}

void TomlWriter::SerializeDouble(f64& value)
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
    if (!SE_ENSURE(value.IsValid(), "Attempting to save invalid TypeId via Text!"))
    {
        SetError("TomlWriter: Attempting to save invalid TypeId.");
    }
    else
    {
        type_name = value.GetName();
    }
    SerializeString(type_name);
}
} // namespace se
