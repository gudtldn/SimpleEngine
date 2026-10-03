#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#include <variant>

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
    [[nodiscard]] virtual bool SupportsRawElements() const override;

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

    /** 맨 위 컨테이너가 Frame이면 그 프레임을, 비었거나 다른 종류면 NullOpt를 돌려줍니다. */
    template <typename Frame>
    [[nodiscard]] Optional<Frame&> TopAs();

private:
    /** Field가 정한 키로 값을 넣는 struct의 테이블 */
    struct StructFrame
    {
        toml::table* table = nullptr;

        /** Field가 정한, 다음 값을 넣을 키 */
        Optional<String> pending_key;

        /** pending_key 자리에 값이 있는 Optional을 열었는지 여부 */
        bool pending_key_in_some = false;
    };

    /** 끝에 값을 넣는 시퀀스의 배열 */
    struct SeqFrame
    {
        toml::array* array = nullptr;

        /** 순서 없는 시퀀스는 EndSeq에서 정렬합니다. */
        ESeqOrder order = ESeqOrder::Ordered;
    };

    /**
     * EndMap에서 테이블이나 쌍 배열로 만들 맵의 엔트리 모음
     * key를 모두 봐야 테이블로 쓸 수 있는지 알 수 있어 EndMap까지 모읍니다.
     */
    struct MapFrame
    {
        /** 지금까지 쓴 [key, value] 배열 */
        toml::array entries;
    };

    /** key와 value를 차례로 넣는 맵 엔트리의 [key, value] 배열 */
    struct MapEntryFrame
    {
        toml::array* pair = nullptr;
    };

    /**
     * 쓰는 중인 컨테이너 하나
     * @note std::variant의 operator<는 제약 없이 선언되어 Deque의 기본 operator<=>가 컴파일되지 않으므로, 구조체로 감쌉니다.
     */
    struct OpenContainer
    {
        std::variant<StructFrame, SeqFrame, MapFrame, MapEntryFrame> frame;
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
    [[nodiscard]] virtual ArrayView<const String> GetWarnings() const override;

public:
    [[nodiscard]] virtual bool IsTextFormat() const override;
    [[nodiscard]] virtual bool SupportsRawElements() const override;

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

    /** 맨 위 컨테이너가 Frame이면 그 프레임을, 비었거나 다른 종류면 NullOpt를 돌려줍니다. */
    template <typename Frame>
    [[nodiscard]] Optional<Frame&> TopAs();

private:
    /** Field가 찾아 둔 값을 꺼내는 struct의 테이블 */
    struct StructFrame
    {
        const toml::table* table = nullptr;

        /** TOML 안의 위치. 루트는 빈 문자열입니다. */
        String path;

        /** Field가 찾아 둔 다음 값 */
        const toml::node* field_value = nullptr;

        /** 타입이 물어본 필드 이름 */
        Array<String> known_keys;
    };

    /** 원소를 차례로 꺼내는 시퀀스의 배열 */
    struct SeqFrame
    {
        const toml::array* array = nullptr;

        /** TOML 안의 위치 */
        String path;

        /** 다음에 꺼낼 원소 번호 */
        usize next_index = 0;
    };

    /** 테이블로 적힌 맵 */
    struct TableMapFrame
    {
        const toml::table* table = nullptr;

        /** TOML 안의 위치 */
        String path;

        /** 다음에 열 엔트리 */
        toml::table::const_iterator next_entry;
    };

    /** [key, value] 쌍 배열로 적힌 맵 */
    struct PairMapFrame
    {
        const toml::array* pairs = nullptr;

        /** TOML 안의 위치 */
        String path;

        /** 다음에 열 쌍의 번호 */
        usize next_index = 0;
    };

    /** 테이블 맵의 엔트리 하나 */
    struct TableEntryFrame
    {
        /** 키를 담은 문자열 노드. key 자리에서 문자열이나 enum 이름으로 읽힙니다. */
        toml::value<std::string> key;

        const toml::node* value = nullptr;

        /** key와 value가 함께 쓰는 엔트리의 위치 (예: "scores.alice") */
        String path;

        /** 다음에 꺼낼 것 (0은 key, 1은 value) */
        usize next_slot = 0;
    };

    /** 쌍 배열 맵의 엔트리 하나 */
    struct PairEntryFrame
    {
        const toml::array* pair = nullptr;

        /** [key, value] 배열의 위치 (예: "points[0]") */
        String path;

        /** 다음에 꺼낼 것 (0은 key, 1은 value) */
        usize next_slot = 0;
    };

    /**
     * 읽는 중인 컨테이너 하나
     * TomlWriter::OpenContainer와 같은 이유로 구조체로 감쌉니다.
     */
    struct OpenContainer
    {
        std::variant<StructFrame, SeqFrame, TableMapFrame, PairMapFrame, TableEntryFrame, PairEntryFrame> frame;
    };

    const toml::table& root;
    Stack<OpenContainer> open_containers;

    /** 테이블에 있는데 타입에 없는 키의 경고 */
    Array<String> warnings;

    /** 루트 struct를 시작했는지 여부 */
    bool root_started = false;
};
} // namespace se
