#include "SimpleEngine/Core/Serialization/PackedArchive.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Utility/Debug.h"

#include <cstring>
#include <limits>


namespace se
{
namespace
{
[[nodiscard]] u32 WidthToBytes(EIntWidth width)
{
    switch (width)
    {
        case EIntWidth::Bits8:  return 1;
        case EIntWidth::Bits16: return 2;
        case EIntWidth::Bits32: return 4;
        case EIntWidth::Bits64: return 8;
    }
    SE_UNREACHABLE();
}
} // namespace


// PackedWriter
PackedWriter::PackedWriter(Array<u8>& out_buffer)
    : buffer(out_buffer)
{
    offset = buffer.Len();
}

void PackedWriter::Int(i64& value, EIntWidth width, [[maybe_unused]] bool is_signed)
{
    // 네이티브(리틀) 엔디안이므로 하위 n바이트만 잘라 쓰면 됨.
    WriteBytes(&value, WidthToBytes(width));
}

void PackedWriter::Float(f64& value, EFloatWidth width)
{
    if (width == EFloatWidth::Bits32)
    {
        const f32 narrowed = static_cast<f32>(value);
        WriteBytes(&narrowed, sizeof(f32));
    }
    else
    {
        WriteBytes(&value, sizeof(f64));
    }
}

void PackedWriter::Bool(bool& value)
{
    WriteBoolByte(value);
}

void PackedWriter::Bytes(void* data, u64 size)
{
    WriteBytes(data, size);
}

void PackedWriter::Enum(i64& value, [[maybe_unused]] ArrayView<const EnumEntry> entries, EIntWidth width)
{
    // 바이너리에서는 entries(이름 테이블)를 쓰지 않고 Int와 동일하게 취급합니다.
    Int(value, width, true);
}

void PackedWriter::BeginStruct() {}

bool PackedWriter::Field([[maybe_unused]] u32 field_id, [[maybe_unused]] StringView name, [[maybe_unused]] u8 wire_type)
{
    return true;
}

void PackedWriter::EndStruct() {}

void PackedWriter::BeginSeq(u64& count)
{
    if (HasError())
    {
        return;
    }
    if (count > std::numeric_limits<u32>::max())
    {
        SetError(String::Format("PackedWriter: sequence count {} exceeds u32 range.", count));
        return;
    }
    const u32 narrowed = static_cast<u32>(count);
    WriteBytes(&narrowed, sizeof(u32));
}

void PackedWriter::EndSeq() {}
void PackedWriter::BeginMapEntry() {}
void PackedWriter::EndMapEntry() {}

void PackedWriter::Present(bool& has_value)
{
    WriteBoolByte(has_value);
}

void PackedWriter::WriteBytes(const void* src, u64 byte_size)
{
    if (HasError())
    {
        return;
    }
    const usize required = offset + byte_size;
    if (required > buffer.Len())
    {
        buffer.ResizeUninitialized(required);
    }
    std::memcpy(buffer.Data() + offset, src, byte_size);
    offset += byte_size;
}

void PackedWriter::WriteBoolByte(bool value)
{
    const u8 byte = value ? 1 : 0;
    WriteBytes(&byte, 1);
}


// PackedReader
PackedReader::PackedReader(ArrayView<const u8> in_view)
    : buffer_view(in_view)
{
}

void PackedReader::Int(i64& value, EIntWidth width, bool is_signed)
{
    if (HasError())
    {
        return;
    }

    const u32 n = WidthToBytes(width);
    i64 raw = 0;
    ReadBytes(&raw, n);
    if (HasError())
    {
        return;
    }

    // NOLINTBEGIN(*-signed-bitwise)
    if (is_signed && n < sizeof(i64))
    {
        // n바이트 폭에서의 최상위 비트를 i64로 확장
        const u32 bits = n * 8;
        const i64 sign_bit = i64{ 1 } << (bits - 1);
        raw = (raw ^ sign_bit) - sign_bit;
    }
    // NOLINTEND(*-signed-bitwise)
    value = raw;
}

void PackedReader::Float(f64& value, EFloatWidth width)
{
    if (HasError())
    {
        return;
    }

    if (width == EFloatWidth::Bits32)
    {
        f32 narrowed = 0.0f;
        ReadBytes(&narrowed, sizeof(f32));
        if (HasError())
        {
            return;
        }
        value = static_cast<f64>(narrowed);
    }
    else
    {
        f64 raw = 0.0;
        ReadBytes(&raw, sizeof(f64));
        if (HasError())
        {
            return;
        }
        value = raw;
    }
}

void PackedReader::Bool(bool& value)
{
    ReadBoolByte(value);
}

void PackedReader::Bytes(void* data, u64 size)
{
    ReadBytes(data, size);
}

void PackedReader::Enum(i64& value, [[maybe_unused]] ArrayView<const EnumEntry> entries, EIntWidth width)
{
    Int(value, width, true);
}

void PackedReader::BeginStruct() {}

bool PackedReader::Field([[maybe_unused]] u32 field_id, [[maybe_unused]] StringView name, [[maybe_unused]] u8 wire_type)
{
    return true;
}

void PackedReader::EndStruct() {}

void PackedReader::BeginSeq(u64& count)
{
    if (HasError())
    {
        return;
    }
    u32 narrowed = 0;
    ReadBytes(&narrowed, sizeof(u32));
    if (HasError())
    {
        return;
    }
    count = narrowed;
}

void PackedReader::EndSeq() {}
void PackedReader::BeginMapEntry() {}
void PackedReader::EndMapEntry() {}

void PackedReader::Present(bool& has_value)
{
    ReadBoolByte(has_value);
}

void PackedReader::ReadBytes(void* dest, u64 byte_size)
{
    if (HasError())
    {
        return;
    }

    if (offset + byte_size > buffer_view.Len())
    {
        SetError(String::Format(
            "PackedReader: buffer overflow. (offset: {}, size: {}, buffer_len: {})",
            offset, byte_size, buffer_view.Len()
        ));
        return;
    }
    std::memcpy(dest, buffer_view.Data() + offset, byte_size);
    offset += byte_size;
}

void PackedReader::ReadBoolByte(bool& value)
{
    if (HasError())
    {
        return;
    }
    u8 byte = 0;
    ReadBytes(&byte, 1);
    if (HasError())
    {
        return;
    }
    value = byte != 0;
}
} // namespace se
