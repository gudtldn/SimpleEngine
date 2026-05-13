#pragma once

#include "SimpleEngine/Core/Container/Array.h"
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
 * TOML 직렬화/역직렬화 시스템의 기반 클래스
 * 트리 구조인 TOML 데이터를 선형적인 C++ 코드 흐름(Serialize 함수 호출)으로
 * 읽고 쓰기 위해, 현재 어디를 탐색 중인지(Context Stack)를 추적합니다.
 */
class SE_CORE_API TomlArchive : public Archive
{
protected:
    /** 현재 탐색 중인 노드의 모드를 나타내는 Enum */
    enum class EContextMode : u8
    {
        Object, // 일반적인 객체 (내부적으로 toml::table 사용)
        Array,  // 배열 (toml::array 사용)
        Map,    // 동적 딕셔너리 (toml::table 사용, Key-Value 쌍 순회)
    };

    /** 현재 탐색 중인 노드의 상태를 나타내는 컨텍스트 */
    struct Context
    {
        toml::node* node = nullptr;
        EContextMode mode = EContextMode::Object;

        // Array 내부 요소들을 읽을 때 사용할 현재 인덱스
        usize array_idx = 0;

        // Map 역직렬화 시, TOML Table 내부의 Key-Value 쌍들을 차례로 읽기 위한 Iterator
        toml::table::iterator map_it;
        toml::table::iterator map_end;

        [[nodiscard]] bool IsObject() const { return mode == EContextMode::Object; }
        [[nodiscard]] bool IsArray() const { return mode == EContextMode::Array; }
        [[nodiscard]] bool IsMap() const { return mode == EContextMode::Map; }
    };

public:
    virtual ~TomlArchive() override = default;

    [[nodiscard]] virtual bool IsBinary() const override { return false; }

protected:
    explicit TomlArchive(EArchiveMode mode) : Archive(mode) {}

    [[nodiscard]] Context& GetCurrentContext();
    [[nodiscard]] bool IsRootNode(const toml::node* node) const;

protected:
    /**
     * 직렬화할 다음 데이터의 Key
     * Table에 데이터를 넣거나 읽기 전, HintNextName()을 통해 미리 설정됩니다.
     * Array 모드일 때는 무시되며, 사용 후에는 반드시 비워져야(Clear) 합니다.
     */
    StringView pending_key;

    /**
     * 계층 구조를 관리하는 Stack
     * BeginObject/Array/Map 호출 시 Push되고, End 호출 시 Pop 됩니다.
     * 인덱스 0번(Front)은 항상 최상위(Root) 노드를 가리킵니다.
     */
    Array<Context> context_stack;
};


/**
 * TOML 데이터를 읽어서 데이터를 객체로 변환하는 역직렬화 클래스
 */
class SE_CORE_API TomlReader : public TomlArchive
{
public:
    explicit TomlReader(const toml::table& root);

    virtual void BeginObject() override;
    virtual void EndObject() override;
    virtual void BeginArray(u64& count) override;
    virtual void EndArray() override;
    virtual void BeginMap(u64& count) override;
    virtual void EndMap() override;
    virtual void BeginMapKey() override;
    virtual void EndMapKey() override;
    virtual void BeginMapValue() override;
    virtual void EndMapValue() override;

    virtual void SerializeBytes(void* data, u64 size) override;

protected:
    virtual void HintNextName(StringView name) override;

    virtual void SerializeBool(bool& value) override;
    virtual void SerializeInt8(i8& value) override;
    virtual void SerializeUInt8(u8& value) override;
    virtual void SerializeInt16(i16& value) override;
    virtual void SerializeUInt16(u16& value) override;
    virtual void SerializeInt32(i32& value) override;
    virtual void SerializeUInt32(u32& value) override;
    virtual void SerializeInt64(i64& value) override;
    virtual void SerializeUInt64(u64& value) override;
    virtual void SerializeFloat(f32& value) override;
    virtual void SerializeDouble(f64& value) override;

    virtual void SerializeString(String& value) override;
    virtual void SerializeStringName(StringName& value) override;
    virtual void SerializeGuid(Guid& value) override;
    virtual void SerializeTypeId(TypeId& value) override;

private:
    /**
     * 현재 Context(배열 인덱스, pending_key)에 맞춰 값을 읽어올 자식 노드를 반환하고,
     * 내부 진행 상태(인덱스 등)를 갱신합니다.
     */
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
    /**
     * Map Key 역직렬화 모드 플래그
     * - true : TOML 노드를 읽지 않고, current_map_key 문자열을 파싱하여 C++ 타입으로 반환
     * - false: 정상적으로 TOML 트리에서 Value 노드를 파싱
     */
    bool reading_map_key = false;

    /** 현재 Map 컨텍스트에서 처리 중인 Key의 문자열 */
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
    virtual void BeginArray(u64& count) override;
    virtual void EndArray() override;
    virtual void BeginMap(u64& count) override;
    virtual void EndMap() override;
    virtual void BeginMapKey() override;
    virtual void EndMapKey() override;
    virtual void BeginMapValue() override;
    virtual void EndMapValue() override;

    virtual void SerializeBytes(void* data, u64 size) override;

protected:
    virtual void HintNextName(StringView name) override;

    virtual void SerializeBool(bool& value) override;
    virtual void SerializeInt8(i8& value) override;
    virtual void SerializeUInt8(u8& value) override;
    virtual void SerializeInt16(i16& value) override;
    virtual void SerializeUInt16(u16& value) override;
    virtual void SerializeInt32(i32& value) override;
    virtual void SerializeUInt32(u32& value) override;
    virtual void SerializeInt64(i64& value) override;
    virtual void SerializeUInt64(u64& value) override;
    virtual void SerializeFloat(f32& value) override;
    virtual void SerializeDouble(f64& value) override;

    virtual void SerializeString(String& value) override;
    virtual void SerializeStringName(StringName& value) override;
    virtual void SerializeGuid(Guid& value) override;
    virtual void SerializeTypeId(TypeId& value) override;

private:
    /**
     * 현재 컨텍스트에 따라 새 데이터를 적절한 방식으로 직렬화 합니다.
     * - Array Mode: 배열 끝에 추가 (push_back)
     * - Table Mode: pending_key를 이름으로 하여 추가 (insert_or_assign)
     */
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
            // Table 모드인데 Key를 지정하지 않고 값을 쓰려는 경우 Assert
            SE_ASSERT(false, "TomlWriter::WriteValue - No valid target. (IsArray: false, pending_key: empty)");
        }
    }

    /**
     * Table이나 Array를 표현할 새로운 빈 노드를 만들고,
     * 현재 계층 구조에 맞게 부모 노드에 부착한 뒤 그 포인터를 반환합니다.
     */
    template <typename NodeType, typename... Args>
    NodeType* InsertNewNode(Args&&... args)
    {
        Context& ctx = GetCurrentContext();

        // Writer의 내부 컨텍스트 노드가 nullptr인 것은 상태 관리 로직이 꼬인 것이므로 Assert
        SE_ASSERT(ctx.node, "TomlWriter::InsertNewNode - Current node is null!");

        if (ctx.IsArray())
        {
            auto& ref = ctx.node->as_array()->emplace_back(std::forward<Args>(args)...);
            return ref.template as<NodeType>();
        }

        // Array가 아닌데 pending_key가 없는 경우 Assert
        SE_ASSERT(!pending_key.IsEmpty(), "TomlWriter::InsertNewNode - pending_key is empty! Cannot insert into table without a key.");

        auto [it, _] = ctx.node->as_table()->insert_or_assign(pending_key, std::forward<Args>(args)...);
        pending_key = "";
        return it->second.template as<NodeType>();
    }

private:
    /**
     * Map Key 직렬화(가로채기) 모드 플래그
     * - true : 노드를 생성하지 않고, 값을 문자열로 변환하여 current_map_key에 저장
     * - false: 정상적으로 TOML 트리에 값을 기록
     */
    bool capturing_map_key = false;

    /** 캡쳐 모드에서 임시로 저장할 Key 문자열 */
    String current_map_key;
};
} // namespace se
