#include "SimpleEngine/Core/Serialization/TomlArchive.h"

#include "Core/Types/Guid.h"
#include "Core/Types/StringName.h"
#include "Utility/Debug.h"


namespace se::core
{
void TomlArchive::HintNextName(const char* name)
{
    pending_key = name;
}

void TomlArchive::ProcessBytes([[maybe_unused]] void* value, [[maybe_unused]] uint64 byte_size)
{
    // TomlArchive는 BinaryData를 사용 안함. (나중에 Base64로 해도 되고)
}

TomlArchive::Context& TomlArchive::GetCurrentContext()
{
    SE_ASSERT(!context_stack.IsEmpty(), "Context stack underflow!");
    return context_stack.Top().Value();
}

TomlReader::TomlReader(const toml::table& root)
    : TomlArchive(EArchiveMode::LoadText)
{
    context_stack.Push({
        .node = const_cast<toml::table*>(&root),
        .array_idx_opt = std::nullopt,
    });
}

void TomlReader::BeginNode()
{
    const Context& ctx = GetCurrentContext();
    if (!ctx.IsArray() && pending_key.empty())
    {
        context_stack.Push(ctx);
        return;
    }

    // 찾은 노드가 테이블이면 스택에 푸시
    toml::node* sub_node = GetCurrentNode();
    if (sub_node && sub_node->is_table())
    {
        context_stack.Push({
            .node = sub_node,
            .array_idx_opt = std::nullopt,
        });
    }
    else
    {
        ConsoleLog(ELogLevel::Warning, "Expected a table but found something else.");
        context_stack.Push({
            .node = nullptr,
            .array_idx_opt = std::nullopt,
        });
    }
}

void TomlReader::EndNode()
{
    context_stack.Pop();
}

void TomlReader::BeginArray(uint64& count)
{
    toml::node* sub_node = GetCurrentNode();
    if (sub_node && sub_node->is_array())
    {
        toml::array* arr = sub_node->as_array();
        count = arr->size();
        context_stack.Push({
            .node = arr,
            .array_idx_opt = 0,
        });
    }
    else
    {
        count = 0;
        context_stack.Push({
            .node = nullptr,
            .array_idx_opt = 0,
        });
    }
}

void TomlReader::EndArray()
{
    context_stack.Pop();
}

#define OVERRIDE_TOML_READ(type) \
Archive& TomlReader::operator<<(type& value) { ReadValue(value); return *this; }

OVERRIDE_TOML_READ(int8)
OVERRIDE_TOML_READ(uint8)
OVERRIDE_TOML_READ(int16)
OVERRIDE_TOML_READ(uint16)
OVERRIDE_TOML_READ(int32)
OVERRIDE_TOML_READ(uint32)
OVERRIDE_TOML_READ(int64)
Archive& TomlReader::operator<<(uint64& value)
{
    int64 temp = 0;
    if (ReadValue(temp))
    {
        value = static_cast<uint64>(temp);
    }
    return *this;
}
OVERRIDE_TOML_READ(float)
OVERRIDE_TOML_READ(double)
OVERRIDE_TOML_READ(bool)

#undef OVERRIDE_TOML_READ

Archive& TomlReader::operator<<(String& value)
{
    std::string_view sv;
    if (ReadValue(sv))
    {
        value = sv;
    }
    return *this;
}

Archive& TomlReader::operator<<(StringName& value)
{
    std::string_view sv;
    if (ReadValue(sv))
    {
        value = sv;
    }
    return *this;
}

Archive& TomlReader::operator<<(Guid& value)
{
    std::string_view sv;
    if (ReadValue(sv))
    {
        value = Guid::FromString(sv);
    }
    return *this;
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
        // 배열 모드: 인덱스로 접근 후 증가
        toml::array* arr = ctx.node->as_array();
        if (*ctx.array_idx_opt < arr->size())
        {
            return arr->get((*ctx.array_idx_opt)++);
        }
        return nullptr; // 인덱스 초과
    }

    // 테이블 모드: 키(Hint)로 접근
    if (!pending_key.empty())
    {
        toml::node* node = ctx.node->as_table()->get(pending_key);
        pending_key = ""; // 키 소비
        return node;
    }
    return nullptr;
}

TomlWriter::TomlWriter(toml::table& root)
    : TomlArchive(EArchiveMode::SaveText)
{
    context_stack.Push({
        .node = &root,
        .array_idx_opt = std::nullopt,
    });
}

void TomlWriter::BeginNode()
{
    const Context& ctx = GetCurrentContext();

    //  현재가 테이블(Table) 내부인데, 키(pending_key)가 없다면 Root 테이블로 취급
    if (!ctx.IsArray() && pending_key.empty())
    {
        context_stack.Push(ctx);
        return;
    }

    context_stack.Push({
        .node = InsertNewNode<toml::table>(toml::table{}),
        .array_idx_opt = std::nullopt,
    });
}

void TomlWriter::EndNode()
{
    context_stack.Pop();
}

void TomlWriter::BeginArray([[maybe_unused]] uint64& count)
{
    context_stack.Push({
        .node = InsertNewNode<toml::array>(toml::array{}),
        .array_idx_opt = 0,
    });
}

void TomlWriter::EndArray()
{
    context_stack.Pop();
}

#define OVERRIDE_TOML_WRITE(type) \
Archive& TomlWriter::operator<<(type& value) { WriteValue(value); return *this; }

OVERRIDE_TOML_WRITE(int8)
OVERRIDE_TOML_WRITE(uint8)
OVERRIDE_TOML_WRITE(int16)
OVERRIDE_TOML_WRITE(uint16)
OVERRIDE_TOML_WRITE(int32)
OVERRIDE_TOML_WRITE(uint32)
OVERRIDE_TOML_WRITE(int64)
Archive& TomlWriter::operator<<(uint64& value)
{
    WriteValue(static_cast<int64_t>(value));
    return *this;
}
OVERRIDE_TOML_WRITE(float)
OVERRIDE_TOML_WRITE(double)
OVERRIDE_TOML_WRITE(bool)

#undef OVERRIDE_TOML_WRITE

Archive& TomlWriter::operator<<(String& value)
{
    WriteValue(value.Bytes());
    return *this;
}

Archive& TomlWriter::operator<<(StringName& value)
{
    WriteValue(value.CStr());
    return *this;
}

Archive& TomlWriter::operator<<(Guid& value)
{
    WriteValue(value.ToString().CStr());
    return *this;
}
}
