#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se
{
/**
 * 태그 없이 순서에만 의존하는 최소 바이너리 포맷으로 씁니다.
 * @warning 스키마 해시가 없어 위치 기반 오염 위험이 있으므로, 같은 프로세스 내 메모리 왕복
 *          (DDC/런타임 캐시) 용도로만 사용하고 디스크에 작성하는 용도로 사용해서는 안됩니다.
 */
class SE_CORE_API PackedWriter final : public Archive
{
public:
    explicit PackedWriter(Array<u8>& out_buffer);

public:
    virtual void Int(i64& value, EIntWidth width, bool is_signed) override;
    virtual void Float(f64& value, EFloatWidth width) override;
    virtual void Bool(bool& value) override;
    virtual void Bytes(void* data, u64 size) override;
    virtual void Enum(i64& value, ArrayView<const EnumEntry> entries, EIntWidth width) override;

    virtual void BeginStruct() override;
    [[nodiscard]] virtual bool Field(u32 field_id, StringView name, u8 wire_type) override;
    virtual void EndStruct() override;
    virtual void BeginSeq(u64& count) override;
    virtual void EndSeq() override;
    virtual void BeginMapEntry() override;
    virtual void EndMapEntry() override;
    virtual void Present(bool& has_value) override;

private:
    void WriteBytes(const void* src, u64 byte_size);
    void WriteBoolByte(bool value);

private:
    Array<u8>& buffer;
    usize offset = 0;
};


/** PackedWriter가 쓴 바이트를 같은 규약으로 되읽습니다. */
class SE_CORE_API PackedReader final : public Archive
{
public:
    explicit PackedReader(ArrayView<const u8> in_view);

public:
    virtual void Int(i64& value, EIntWidth width, bool is_signed) override;
    virtual void Float(f64& value, EFloatWidth width) override;
    virtual void Bool(bool& value) override;
    virtual void Bytes(void* data, u64 size) override;
    virtual void Enum(i64& value, ArrayView<const EnumEntry> entries, EIntWidth width) override;

    virtual void BeginStruct() override;
    [[nodiscard]] virtual bool Field(u32 field_id, StringView name, u8 wire_type) override;
    virtual void EndStruct() override;
    virtual void BeginSeq(u64& count) override;
    virtual void EndSeq() override;
    virtual void BeginMapEntry() override;
    virtual void EndMapEntry() override;
    virtual void Present(bool& has_value) override;

private:
    void ReadBytes(void* dest, u64 byte_size);
    void ReadBoolByte(bool& value);

private:
    ArrayView<const u8> buffer_view;
    usize offset = 0;
};
} // namespace se
