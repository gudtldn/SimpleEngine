#pragma once
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"
#undef TOML_EXCEPTIONS


namespace se
{
/**
 * toml++ 라이브러리를 기반으로 하는 TOML 직렬화 시스템의 기반 클래스
 * 현재 탐색 중인 노드의 위치(Context) 관리와 키(Key)와 값(Value)의 매핑 상태를 추적합니다.
 */
class SE_CORE_API TomlArchive : public Archive
{
protected:
    struct Context
    {
        toml::node* node = nullptr;    // 현재 포커스된 노드 (Table or Array)
        Optional<usize> array_idx_opt; // 배열 인덱스

        [[nodiscard]] bool IsArray() const { return array_idx_opt.HasValue(); }
    };

public:
    virtual ~TomlArchive() override = default;

    /** 다음에 처리할 필드(Key)의 이름을 임시 저장합니다. */
    virtual void HintNextName(const char* name) override;

protected:
    explicit TomlArchive(EArchiveMode mode) : Archive(mode) {}
    virtual void ProcessBytes(void* value, uint64 byte_size) override;

    [[nodiscard]] Context& GetCurrentContext();

protected:
    StringView pending_key;
    Stack<Context> context_stack;
};

/**
 * TOML 데이터 구조를 순회하며 값을 읽어오는 역직렬화(Deserialization) 클래스
 * toml++의 노드 트리에서 데이터를 추출하여 엔진의 타입으로 변환합니다.
 */
class SE_CORE_API TomlReader : public TomlArchive
{
public:
    explicit TomlReader(const toml::table& root);

    virtual void BeginNode() override;
    virtual void EndNode() override;
    virtual void BeginArray(uint64& count) override;
    virtual void EndArray() override;

    virtual Archive& operator<<(int8& value) override;
    virtual Archive& operator<<(uint8& value) override;
    virtual Archive& operator<<(int16& value) override;
    virtual Archive& operator<<(uint16& value) override;
    virtual Archive& operator<<(int32& value) override;
    virtual Archive& operator<<(uint32& value) override;
    virtual Archive& operator<<(int64& value) override;
    virtual Archive& operator<<(uint64& value) override;
    virtual Archive& operator<<(float& value) override;
    virtual Archive& operator<<(double& value) override;
    virtual Archive& operator<<(bool& value) override;

    virtual Archive& operator<<(String& value) override;
    virtual Archive& operator<<(StringName& value) override;
    virtual Archive& operator<<(Guid& value) override;

private:
    toml::node* GetCurrentNode();

    template <typename T>
    bool ReadValue(T& out_val)
    {
        if (toml::node* node = GetCurrentNode())
        {
            // toml++의 value_or 혹은 value<T> 사용
            if (auto val = node->value<T>())
            {
                out_val = *val;
                return true;
            }
        }
        return false;
    }
};

/**
 * 데이터를 TOML 포맷으로 변환하여 저장하는 직렬화(Serialization) 클래스
 * 데이터 구조에 따라 Table이나 Array 노드를 동적으로 생성하고 값을 삽입합니다.
 */
class SE_CORE_API TomlWriter : public TomlArchive
{
public:
    explicit TomlWriter(toml::table& root);

    virtual void BeginNode() override;
    virtual void EndNode() override;
    virtual void BeginArray(uint64& count) override;
    virtual void EndArray() override;

    virtual Archive& operator<<(int8& value) override;
    virtual Archive& operator<<(uint8& value) override;
    virtual Archive& operator<<(int16& value) override;
    virtual Archive& operator<<(uint16& value) override;
    virtual Archive& operator<<(int32& value) override;
    virtual Archive& operator<<(uint32& value) override;
    virtual Archive& operator<<(int64& value) override;
    virtual Archive& operator<<(uint64& value) override;
    virtual Archive& operator<<(float& value) override;
    virtual Archive& operator<<(double& value) override;
    virtual Archive& operator<<(bool& value) override;

    virtual Archive& operator<<(String& value) override;
    virtual Archive& operator<<(StringName& value) override;
    virtual Archive& operator<<(Guid& value) override;

private:
    template <typename T>
    void WriteValue(T&& val)
    {
        Context& ctx = GetCurrentContext();
        if (ctx.IsArray())
        {
            // 배열이면 push_back
            ctx.node->as_array()->push_back(std::forward<T>(val));
        }
        else
        {
            // 테이블이면 Key-Value 삽입
            if (!pending_key.IsEmpty())
            {
                ctx.node->as_table()->insert_or_assign(pending_key, std::forward<T>(val));
                pending_key = ""; // 키 소비
            }
        }
    }

    /** 테이블이나 배열 같은 컨테이너 노드를 생성해서 삽입하고 포인터 반환 */
    template <typename NodeType, typename... Args>
    NodeType* InsertNewNode(Args&&... args)
    {
        Context& ctx = GetCurrentContext();

        if (!ctx.node)
        {
            ConsoleLog(ELogLevel::Error, "Current node is null!");
            return nullptr;
        }

        if (ctx.IsArray())
        {
            auto& ref = ctx.node->as_array()->emplace_back(std::forward<Args>(args)...);
            return ref.template as<NodeType>();
        }

        if (pending_key.IsEmpty())
        {
            ConsoleLog(ELogLevel::Error, "Pending key is empty!");
            return nullptr;
        }
        auto [it, _] = ctx.node->as_table()->insert_or_assign(pending_key, std::forward<Args>(args)...);
        pending_key = "";
        return it->second.template as<NodeType>();
    }
};
}  // namespace se
