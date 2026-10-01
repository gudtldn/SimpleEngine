#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#define TOML_EXCEPTIONS 0
#include "toml++/toml.h"
#undef TOML_EXCEPTIONS


namespace se
{
/**
 * 값을 toml++ 테이블에 씁니다.
 * 루트 값은 struct 하나여야 합니다(TOML의 최상위는 테이블). struct는 테이블, 필드는 키, 시퀀스는 배열, Bytes는 base64 문자열이 됩니다.
 * 맵은 key가 모두 문자열 하나로 쓰이면 테이블, 아니면 [key, value] 쌍 배열이 됩니다. 순서 없는 시퀀스와 쌍 배열은 정렬해서 씁니다.
 * Optional의 None은 struct 필드에서만 키를 생략해 쓰고, 시퀀스 원소, 맵의 key와 value, 다른 Optional 안이면 오류를 남깁니다.
 */
class SE_CORE_API TomlWriter final : public ArchiveWriter
{
public:
    /** 루트 struct의 필드를 out_root에 씁니다. 같은 이름의 키가 이미 있으면 덮어씁니다. */
    explicit TomlWriter(toml::table& out_root);

public:
    [[nodiscard]] virtual bool IsTextFormat() const override;

    virtual void Int(i64 value, EIntWidth width, bool is_signed) override;
    virtual void Float(f64 value, EFloatWidth width) override;
    virtual void Bool(bool value) override;
    virtual void Str(StringView value) override;
    virtual void Bytes(const void* data, u64 size) override;
    virtual void Enum(i64 value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries) override;

    virtual void BeginStruct() override;
    virtual void Field(StringView name) override;
    virtual void EndStruct() override;
    virtual void BeginSeq(u64 count, ESeqOrder order) override;
    virtual void EndSeq() override;
    virtual void BeginMap(u64 count) override;
    virtual void BeginMapEntry() override;
    virtual void EndMapEntry() override;
    virtual void EndMap() override;
    virtual void Present(bool has_value) override;
    virtual void RawElements(const void* data, u64 size) override;

private:
    /**
     * 지금 열린 컨테이너에 값 하나를 넣을 수 있는지 확인합니다.
     * struct면 Field로 키를 정했어야 하고, 맵이면 BeginMapEntry로 엔트리를 열었어야 합니다. 넣을 수 없으면 SetError를 호출하고 false를 돌려줍니다.
     */
    [[nodiscard]] bool CanPlaceValue();

    /**
     * value를 지금 열린 struct의 테이블(Field가 정한 키) 또는 배열(끝)에 넣고, 넣은 노드를 돌려줍니다.
     * 넣을 곳이 없으면 SetError를 호출하고 nullptr를 돌려줍니다.
     */
    template <typename Value>
    toml::node* PlaceValue(Value&& value);

private:
    /** 쓰는 중인 컨테이너의 종류 */
    enum class EContainerKind : u8
    {
        /** struct. 테이블에 Field가 정한 키로 값을 넣습니다. */
        Struct,

        /** 순서 있는 시퀀스. 배열 끝에 값을 넣습니다. */
        Seq,

        /** 순서 없는 시퀀스. 배열 끝에 값을 넣고 EndSeq에서 정렬합니다. */
        UnorderedSeq,

        /** 맵. 엔트리를 모아 두었다가 EndMap에서 테이블이나 쌍 배열로 만들어 넣습니다. */
        Map,

        /** 맵 엔트리 하나. [key, value] 배열에 key와 value를 차례로 넣습니다. */
        MapEntry,
    };

    /** 쓰는 중인 컨테이너 하나 */
    struct OpenContainer
    {
        EContainerKind kind = EContainerKind::Struct;

        /** 값을 넣을 테이블이나 배열. Map이면 nullptr입니다. */
        toml::node* node = nullptr;

        /** Struct일 때 Field가 정한, 다음 값을 넣을 키 */
        Optional<String> pending_key;

        /** Struct일 때 pending_key 자리에 값이 있는 Optional을 열었는지 여부 */
        bool pending_key_in_some = false;

        /** Map일 때 모은 [key, value] 배열. key를 모두 봐야 테이블로 쓸 수 있는지 알 수 있어 EndMap까지 모읍니다. */
        toml::array map_entries;
    };

    toml::table& root;
    Stack<OpenContainer> open_containers;

    /** 루트 struct를 시작했는지 여부 */
    bool root_started = false;
};


/**
 * toml++ 테이블에서 값을 읽습니다.
 * 테이블에 없는 필드는 Field가 false를 돌려줍니다. 값의 TOML 종류가 다르거나 범위를 벗어나면 오류를 남깁니다.
 * 테이블에 있는데 타입에 없는 키는 오류가 아니라 경고로 남기고 읽기를 계속합니다.
 * 맵은 테이블과 [key, value] 쌍 배열을 모두 받고, 테이블의 키는 문자열 노드처럼 읽힙니다(문자열, enum 이름 등).
 */
class SE_CORE_API TomlReader final : public ArchiveReader
{
public:
    /** in_root를 루트 struct의 테이블로 보고 읽습니다. */
    explicit TomlReader(const toml::table& in_root);

    /** 테이블에 있는데 타입에 없는 키의 경고를 돌려줍니다. */
    [[nodiscard]] ArrayView<const String> GetWarnings() const;

public:
    [[nodiscard]] virtual bool IsTextFormat() const override;

    virtual void Int(i64& value, EIntWidth width, bool is_signed) override;
    virtual void Float(f64& value, EFloatWidth width) override;
    virtual void Bool(bool& value) override;
    virtual void Str(String& value) override;
    virtual void Bytes(void* data, u64 size) override;
    virtual void Enum(i64& value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries) override;

    virtual void BeginStruct() override;
    [[nodiscard]] virtual bool Field(StringView name) override;
    virtual void EndStruct() override;
    virtual void BeginSeq(u64& count) override;
    virtual void EndSeq() override;
    virtual void BeginMap(u64& count) override;
    virtual void BeginMapEntry() override;
    virtual void EndMapEntry() override;
    virtual void EndMap() override;
    virtual void Present(bool& has_value) override;
    virtual void RawElements(void* data, u64 size) override;

private:
    /**
     * 다음 값 하나를 꺼냅니다. struct면 Field가 찾아 둔 값, 시퀀스면 다음 원소, 맵 엔트리면 key 다음에 value입니다.
     * 꺼낼 값이 없으면 SetError를 호출하고 nullptr를 돌려줍니다.
     */
    [[nodiscard]] const toml::node* TakeValue();

    /**
     * node를 width와 부호에 맞는 정수로 읽습니다. i64를 넘는 u64는 10진 문자열로 받습니다.
     * 종류가 다르거나 범위를 벗어나면 SetError를 호출하고 NullOpt를 돌려줍니다.
     */
    [[nodiscard]] Optional<i64> ReadInteger(const toml::node& node, EIntWidth width, bool is_signed);

    /**
     * TakeValue로 방금 꺼낸 값의 TOML 안 위치를 만듭니다.
     * struct면 "부모.키", 시퀀스면 "부모[번호]", 테이블 맵의 엔트리면 "맵.키", 쌍 배열 맵의 엔트리면 "맵[번호][0 또는 1]"입니다.
     * 예: "window", "items[1]", "scores.alice", "points[0][1]"
     */
    [[nodiscard]] String PathOfTakenValue() const;

private:
    /** 읽는 중인 컨테이너의 종류 */
    enum class EContainerKind : u8
    {
        /** struct. Field가 테이블에서 찾아 둔 값을 꺼냅니다. */
        Struct,

        /** 시퀀스. 배열 원소를 차례로 꺼냅니다. */
        Seq,

        /** 맵. 테이블이나 [key, value] 쌍 배열이고, BeginMapEntry가 엔트리를 하나씩 엽니다. */
        Map,

        /** 맵 엔트리 하나. key와 value를 차례로 꺼냅니다. */
        MapEntry,
    };

    /** 읽는 중인 컨테이너 하나 */
    struct OpenContainer
    {
        EContainerKind kind = EContainerKind::Struct;

        /** 읽을 테이블이나 배열. MapEntry이면 쌍 배열 맵에서는 [key, value] 배열, 테이블 맵에서는 value입니다. */
        const toml::node* node = nullptr;

        /** TOML 안의 위치. 루트는 빈 문자열입니다. */
        String path;

        /** Struct일 때 Field가 찾아 둔 다음 값 */
        const toml::node* field_value = nullptr;

        /** Struct일 때 타입이 물어본 필드 이름. */
        Array<String> known_keys;

        /** Seq와 쌍 배열 Map일 때 다음에 읽을 원소 번호, MapEntry일 때 다음에 꺼낼 것(0은 key, 1은 value) */
        usize next_index = 0;

        /** 테이블 Map일 때 다음에 열 엔트리 */
        toml::table::const_iterator next_entry;

        /** 테이블 맵의 MapEntry일 때 키를 담은 문자열 노드. key 자리에서 문자열이나 enum 이름으로 읽힙니다. */
        Optional<toml::value<std::string>> table_key;
    };

    const toml::table& root;
    Stack<OpenContainer> open_containers;

    /** 테이블에 있는데 타입에 없는 키의 경고 */
    Array<String> warnings;

    /** 루트 struct를 시작했는지 여부 */
    bool root_started = false;
};
} // namespace se
