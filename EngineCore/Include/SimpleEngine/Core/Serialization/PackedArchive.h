#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se
{
/**
 * 태그 없이 순서에만 의존하는 바이너리 포맷으로 씁니다.
 */
class SE_CORE_API PackedWriter final : public ArchiveWriter
{
public:
    explicit PackedWriter(Array<u8>& out_buffer);

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
    void WriteBytes(const void* src, u64 byte_size);
    void WriteCount(u64 count);
    void WriteBoolByte(bool value);

private:
    Array<u8>& buffer;
    usize offset = 0;
};


/**
 * PackedWriter가 쓴 바이트를 같은 규약으로 되읽습니다.
 */
class SE_CORE_API PackedReader final : public ArchiveReader
{
public:
    explicit PackedReader(ArrayView<const u8> in_view);

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
    void ReadBytes(void* dest, u64 byte_size);

    /**
     * count 접두를 읽고, 남은 바이트 수를 넘지 않는지 검증합니다.
     * 실패하면 count를 건드리지 않습니다.
     */
    [[nodiscard]] bool ReadCount(u64& count);

    void ReadBoolByte(bool& value);

private:
    ArrayView<const u8> buffer_view;
    usize offset = 0;
};
} // namespace se
