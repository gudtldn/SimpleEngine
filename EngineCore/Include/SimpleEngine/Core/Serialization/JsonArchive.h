#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#include <memory>


namespace se
{
/**
 * 노드 호출을 JSON 문서로 씁니다.
 * 루트 값은 어떤 노드든 하나만 쓸 수 있습니다. struct는 필드 선언 순서의 객체, 시퀀스는 배열, Bytes는 base64 문자열이 됩니다.
 * 맵은 key가 모두 문자열 하나로 쓰이면 키 순서의 객체, 아니면 정렬한 [key, value] 쌍 배열이 됩니다. 순서 없는 시퀀스도 정렬해서 씁니다.
 * 절댓값이 2^53 - 1을 넘는 정수는 JSON 숫자로 정확히 읽히지 않으므로 10진 문자열로 씁니다. NaN과 무한대는 쓸 수 없습니다.
 * Optional의 None은 struct 필드에서만 키를 생략해 쓰고, 시퀀스 원소, 맵의 key와 value, 다른 Optional 안이면 오류를 남깁니다.
 */
class SE_CORE_API JsonWriter final : public ArchiveWriter
{
public:
    JsonWriter();
    virtual ~JsonWriter() override;

    /** 쓴 문서를 텍스트로 돌려줍니다. 쓰기 중에 오류가 있었거나, 닫히지 않은 컨테이너가 있거나, 쓴 값이 없으면 오류를 돌려줍니다. */
    [[nodiscard]] Expected<String, String> ToText() const;

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
    virtual void BeginSection() override;
    virtual void EndSection() override;

private:
    /** yyjson 문서와 열린 컨테이너. 헤더에 yyjson을 드러내지 않으려고 .cpp에 정의합니다. */
    struct State;
    std::unique_ptr<State> state;
};


/**
 * JSON 텍스트를 파싱해 노드 단위로 읽습니다.
 * 표준 JSON만 받습니다(주석, 끝 쉼표, NaN과 무한대 리터럴은 파싱 오류). 같은 객체 안의 중복 키도 오류입니다.
 * 객체에 없는 필드는 Field가 false를 돌려줍니다. 값의 JSON 종류가 다르거나 범위를 벗어나면 오류를 남깁니다.
 * 객체에 있는데 타입에 없는 키는 오류가 아니라 경고로 남기고 읽기를 계속합니다.
 * 정수는 JSON 숫자와 10진 문자열을 모두 받고, 맵은 객체와 [key, value] 쌍 배열을 모두 받습니다.
 */
class SE_CORE_API JsonReader final : public ArchiveReader
{
public:
    /** text를 파싱합니다. 실패하면 줄과 열을 담은 오류 상태로 시작합니다. */
    explicit JsonReader(StringView text);
    virtual ~JsonReader() override;

    /** 객체에 있는데 타입에 없는 키의 경고를 돌려줍니다. */
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
    virtual void BeginSection() override;
    virtual void EndSection() override;
    virtual void SkipSection() override;
    virtual void Rewind() override;

private:
    /** yyjson 문서와 열린 컨테이너. 헤더에 yyjson을 드러내지 않으려고 .cpp에 정의합니다. */
    struct State;
    std::unique_ptr<State> state;
};
} // namespace se
