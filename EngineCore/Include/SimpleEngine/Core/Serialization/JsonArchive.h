#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#include <memory>


namespace se
{
/**
 * JSON으로 값을 작성합니다.
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
    struct State;
    std::unique_ptr<State> state;
};


/**
 * JSON 텍스트를 파싱해 읽습니다.
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
    struct State;
    std::unique_ptr<State> state;
};
} // namespace se
