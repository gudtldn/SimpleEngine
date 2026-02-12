#pragma once
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Utility/Debug.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"
#undef TOML_EXCEPTIONS


namespace se
{
/**
 * toml++ 라이브러리 기반의 TOML 직렬화 시스템의 기반 클래스
 * 노드 탐색 컨텍스트 스택과 Key 매핑 상태를 관리합니다.
 */
class SE_CORE_API TomlArchive : public Archive
{
protected:
    /** 현재 탐색 중인 노드의 모드를 나타내는 Enum */
    enum class EMode : uint8
    {
        None,
        ArrayMode,
        MapMode,
    };

    /** 현재 탐색 중인 노드의 상태를 나타내는 컨텍스트 */
    struct Context
    {
        toml::node* node = nullptr; // 현재 포커스된 노드 (Table or Array)
        EMode mode = EMode::None;

        usize array_idx = 0;           // 배열 인덱스 (ArrayMode일 때만 유효)
        toml::table::iterator map_it;  // 현재 순회 중인 Map iterator (MapMode일 때만 유효)
        toml::table::iterator map_end; // Map 끝 iterator (MapMode일 때만 유효)

        [[nodiscard]] bool IsArray() const { return mode == EMode::ArrayMode; }
        [[nodiscard]] bool IsMap() const { return mode == EMode::MapMode; }
    };

public:
    virtual ~TomlArchive() override = default;

    [[nodiscard]] virtual bool IsBinary() const override { return false; }

protected:
    explicit TomlArchive(EArchiveMode mode) : Archive(mode) {}

    [[nodiscard]] Context& GetCurrentContext();

protected:
    /** 다음 노드 접근 시 사용할 TOML Table의 Key */
    StringView pending_key;

    /** TOML 직렬화 시스템에서 노드 탐색 상태를 관리하기 위한 Context Stack */
    Stack<Context> context_stack;
};


/**
 * TOML 데이터를 읽어서 엔진 타입으로 변환하는 역직렬화 클래스
 */
class SE_CORE_API TomlReader : public TomlArchive
{
public:
    explicit TomlReader(const toml::table& root);

    virtual void BeginObject() override;
    virtual void EndObject() override;
    virtual void BeginArray(uint64& count) override;
    virtual void EndArray() override;
    virtual void BeginMap(uint64& count) override;
    virtual void EndMap() override;
    virtual void BeginMapKey() override;
    virtual void EndMapKey() override;
    virtual void BeginMapValue() override;
    virtual void EndMapValue() override;

    virtual void SerializeBytes(void* data, uint64 size) override;

protected:
    virtual void HintNextName(const char* name) override;

    virtual void SerializeBool(bool& value) override;
    virtual void SerializeInt8(int8& value) override;
    virtual void SerializeUInt8(uint8& value) override;
    virtual void SerializeInt16(int16& value) override;
    virtual void SerializeUInt16(uint16& value) override;
    virtual void SerializeInt32(int32& value) override;
    virtual void SerializeUInt32(uint32& value) override;
    virtual void SerializeInt64(int64& value) override;
    virtual void SerializeUInt64(uint64& value) override;
    virtual void SerializeFloat(float& value) override;
    virtual void SerializeDouble(double& value) override;

    virtual void SerializeString(String& value) override;
    virtual void SerializeStringName(StringName& value) override;
    virtual void SerializeGuid(Guid& value) override;
    virtual void SerializeTypeId(TypeId& value) override;

private:
    /** 현재 컨텍스트에서 다음 노드를 가져옵니다. */
    toml::node* GetCurrentNode();

    template <typename T>
    bool ReadValue(T& out_val)
    {
        if (const toml::node* node = GetCurrentNode())
        {
            if (auto val = node->value<T>())
            {
                out_val = *val;
                return true;
            }
        }
        return false;
    }

private:
    /** Map key 읽기 모드인지 여부 */
    bool reading_map_key = false;

    /** Map 읽기에서 현재 key를 가져옵니다. */
    String current_map_key;
};


/**
 * 데이터를 TOML 포맷으로 직렬화하는 클래스
 */
class SE_CORE_API TomlWriter : public TomlArchive
{
public:
    explicit TomlWriter(toml::table& root);

    virtual void BeginObject() override;
    virtual void EndObject() override;
    virtual void BeginArray(uint64& count) override;
    virtual void EndArray() override;
    virtual void BeginMap(uint64& count) override;
    virtual void EndMap() override;
    virtual void BeginMapKey() override;
    virtual void EndMapKey() override;
    virtual void BeginMapValue() override;
    virtual void EndMapValue() override;

    virtual void SerializeBytes(void* data, uint64 size) override;

protected:
    virtual void HintNextName(const char* name) override;

    virtual void SerializeBool(bool& value) override;
    virtual void SerializeInt8(int8& value) override;
    virtual void SerializeUInt8(uint8& value) override;
    virtual void SerializeInt16(int16& value) override;
    virtual void SerializeUInt16(uint16& value) override;
    virtual void SerializeInt32(int32& value) override;
    virtual void SerializeUInt32(uint32& value) override;
    virtual void SerializeInt64(int64& value) override;
    virtual void SerializeUInt64(uint64& value) override;
    virtual void SerializeFloat(float& value) override;
    virtual void SerializeDouble(double& value) override;

    virtual void SerializeString(String& value) override;
    virtual void SerializeStringName(StringName& value) override;
    virtual void SerializeGuid(Guid& value) override;
    virtual void SerializeTypeId(TypeId& value) override;

private:
    template <typename T>
    void WriteValue(T&& val)
    {
        Context& ctx = GetCurrentContext();
        if (ctx.IsArray())
        {
            ctx.node->as_array()->push_back(std::forward<T>(val));
        }
        else if (!pending_key.IsEmpty())
        {
            ctx.node->as_table()->insert_or_assign(pending_key, std::forward<T>(val));
            pending_key = "";
        }
        else
        {
            SE_ENSURE(false, "TomlWriter::WriteValue - No valid target. (IsArray: false, pending_key: empty)");
        }
    }

    /** 컨테이너 노드를 생성하여 삽입하고 포인터를 반환합니다. */
    template <typename NodeType, typename... Args>
    NodeType* InsertNewNode(Args&&... args)
    {
        Context& ctx = GetCurrentContext();

        if (!SE_ENSURE(ctx.node, "TomlWriter::InsertNewNode - Current node is null!"))
        {
            return nullptr;
        }

        if (ctx.IsArray())
        {
            auto& ref = ctx.node->as_array()->emplace_back(std::forward<Args>(args)...);
            return ref.template as<NodeType>();
        }

        if (!SE_ENSURE(!pending_key.IsEmpty(), "TomlWriter::InsertNewNode - pending_key is empty! Cannot insert into table without a key."))
        {
            return nullptr;
        }

        auto [it, _] = ctx.node->as_table()->insert_or_assign(pending_key, std::forward<Args>(args)...);
        pending_key = "";
        return it->second.template as<NodeType>();
    }

private:
    /** Map key 캡처 모드인지 여부 */
    bool capturing_map_key = false;

    /** Map 쓰기에서 현재 key를 저장합니다. */
    String current_map_key;
};
}  // namespace se
