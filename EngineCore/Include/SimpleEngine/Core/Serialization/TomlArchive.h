#pragma once

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
 * 루트 값은 struct 하나여야 합니다(TOML 문서의 최상위는 테이블). struct는 테이블, 필드는 키, 시퀀스는 배열이 됩니다.
 * @note Map, Optional, Bytes, 순서 없는 시퀀스는 아직 지원하지 않아 오류를 남깁니다.
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

private:
    /**
     * value를 지금 열린 테이블(Field가 정한 키) 또는 배열(끝)에 넣고, 넣은 노드를 돌려줍니다.
     * 넣을 곳이 없으면 SetError를 호출하고 nullptr를 돌려줍니다.
     */
    template <typename Value>
    toml::node* PlaceValue(Value&& value);

private:
    /** 쓰는 중인 테이블이나 배열 하나 */
    struct OpenContainer
    {
        toml::node* node = nullptr;

        /** 테이블일 때 Field가 정한, 다음 값을 넣을 키 */
        Optional<String> pending_key;
    };

    toml::table& root;
    Stack<OpenContainer> open_containers;

    /** 루트 struct를 시작했는지 여부 */
    bool root_started = false;
};


/**
 * toml++ 테이블에서 값을 읽습니다. 파일을 파싱하는 일은 호출자가 합니다.
 * 테이블에 없는 필드는 Field가 false를 돌려줍니다. 값의 TOML 종류가 다르거나 범위를 벗어나면 오류를 남깁니다.
 * @note Map, Optional, Bytes는 아직 지원하지 않아 오류를 남깁니다.
 */
class SE_CORE_API TomlReader final : public ArchiveReader
{
public:
    /** in_root를 루트 struct의 테이블로 보고 읽습니다. */
    explicit TomlReader(const toml::table& in_root);

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

private:
    /**
     * 다음 값 하나를 꺼냅니다. 테이블이면 Field가 찾아 둔 값, 배열이면 다음 원소입니다.
     * 꺼낼 값이 없으면 SetError를 호출하고 nullptr를 돌려줍니다.
     */
    [[nodiscard]] const toml::node* TakeValue();

    /**
     * node를 width와 부호에 맞는 정수로 읽습니다. i64를 넘는 u64는 10진 문자열로 받습니다.
     * 종류가 다르거나 범위를 벗어나면 SetError를 호출하고 NullOpt를 돌려줍니다.
     */
    [[nodiscard]] Optional<i64> ReadInteger(const toml::node& node, EIntWidth width, bool is_signed);

private:
    /** 읽는 중인 테이블이나 배열 하나 */
    struct OpenContainer
    {
        const toml::node* node = nullptr;

        /** 테이블일 때 Field가 찾아 둔 다음 값 */
        const toml::node* field_value = nullptr;

        /** 배열일 때 다음에 읽을 원소 번호 */
        usize next_index = 0;
    };

    const toml::table& root;
    Stack<OpenContainer> open_containers;

    /** 루트 struct를 시작했는지 여부 */
    bool root_started = false;
};
} // namespace se
