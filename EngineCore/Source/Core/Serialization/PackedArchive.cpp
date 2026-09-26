#include "SimpleEngine/Core/Serialization/PackedArchive.h"

#include "SimpleEngine/Core/Container/String.h"

#include <xxhash.h>

#include <bit>
#include <cstring>
#include <limits>


namespace se
{
// Int/Float를 바이트 순서 변환 없이 memcpy로 쓰고 읽으므로 리틀 엔디언 플랫폼만 지원합니다.
// 나중에 빅엔디언을 지원하려면 Int/Float의 쓰기와 읽기에 std::byteswap을 넣어야 합니다.
static_assert(std::endian::native == std::endian::little, "PackedArchive only supports little-endian platforms.");

namespace
{
/** 헤더 맨 앞의 식별 바이트 */
constexpr u8 HEADER_MAGIC[4] = { 'S', 'E', 'P', 'K' };

/** 헤더 배치와 노드 인코딩의 버전. 둘 중 하나라도 바뀌면 올립니다. */
constexpr u32 WIRE_VERSION = 1;

/** payload의 체크섬 */
[[nodiscard]] u64 ChecksumOf(ArrayView<const u8> payload)
{
    return XXH3_64bits(payload.Data(), payload.Len());
}
} // namespace


// PackedWriter
PackedWriter::PackedWriter(Array<u8>& out_buffer)
    : buffer(out_buffer)
{
    offset = buffer.Len();
}

bool PackedWriter::IsTextFormat() const
{
    return false;
}

void PackedWriter::Int(i64 value, EIntWidth width, [[maybe_unused]] bool is_signed)
{
    // 리틀 엔디안이므로 하위 n바이트만 잘라 쓰면 됨
    WriteBytes(&value, serde::ByteSizeOf(width));
}

void PackedWriter::Float(f64 value, EFloatWidth width)
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

void PackedWriter::Bool(bool value)
{
    WriteBoolByte(value);
}

void PackedWriter::Str(StringView value)
{
    WriteCount(value.ByteLen());
    WriteBytes(value.Data(), value.ByteLen());
}

void PackedWriter::Bytes(const void* data, u64 size)
{
    WriteBytes(data, size);
}

void PackedWriter::Enum(i64 value, EIntWidth width, bool is_signed, [[maybe_unused]] ArrayView<const EnumEntry> entries)
{
    // 바이너리에서는 entries(이름 테이블)를 쓰지 않고 Int와 동일하게 취급
    Int(value, width, is_signed);
}

void PackedWriter::BeginStruct() {}
void PackedWriter::Field([[maybe_unused]] StringView name) {}
void PackedWriter::EndStruct() {}

void PackedWriter::BeginSeq(u64 count, [[maybe_unused]] ESeqOrder order)
{
    // Packed는 순서를 되살릴 필요가 없으므로 ESeqOrder를 무시
    WriteCount(count);
}

void PackedWriter::EndSeq() {}

void PackedWriter::BeginMap(u64 count)
{
    WriteCount(count);
}

void PackedWriter::BeginMapEntry() {}
void PackedWriter::EndMapEntry() {}
void PackedWriter::EndMap() {}

void PackedWriter::Present(bool has_value)
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

void PackedWriter::WriteCount(u64 count)
{
    if (HasError())
    {
        return;
    }
    if (count > std::numeric_limits<u32>::max())
    {
        SetError(String::Format("PackedWriter: count {} exceeds u32 range.", count));
        return;
    }
    const u32 narrowed = static_cast<u32>(count);
    WriteBytes(&narrowed, sizeof(u32));
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

bool PackedReader::IsTextFormat() const
{
    return false;
}

void PackedReader::Int(i64& value, EIntWidth width, bool is_signed)
{
    if (HasError())
    {
        return;
    }

    const usize n = serde::ByteSizeOf(width);
    i64 raw = 0;
    ReadBytes(&raw, n);
    if (HasError())
    {
        return;
    }

    // NOLINTBEGIN(*-signed-bitwise)
    if (is_signed && n < sizeof(i64))
    {
        // n바이트 폭에서의 최상위 비트를 i64로 부호 확장
        const usize bits = n * 8;
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

void PackedReader::Str(String& value)
{
    u64 length = 0;
    if (!ReadCount(length))
    {
        return;
    }

    value.ResizeUninitialized(length);
    ReadBytes(value.Data(), length);
}

void PackedReader::Bytes(void* data, u64 size)
{
    ReadBytes(data, size);
}

void PackedReader::Enum(i64& value, EIntWidth width, bool is_signed, [[maybe_unused]] ArrayView<const EnumEntry> entries)
{
    Int(value, width, is_signed);
}

void PackedReader::BeginStruct() {}

bool PackedReader::Field([[maybe_unused]] StringView name)
{
    return true;
}

void PackedReader::EndStruct() {}

void PackedReader::BeginSeq(u64& count)
{
    if (u64 result = 0; ReadCount(result))
    {
        count = result;
    }
}

void PackedReader::EndSeq() {}

void PackedReader::BeginMap(u64& count)
{
    if (u64 result = 0; ReadCount(result))
    {
        count = result;
    }
}

void PackedReader::BeginMapEntry() {}
void PackedReader::EndMapEntry() {}
void PackedReader::EndMap() {}

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

    if (byte_size > buffer_view.Len() - offset)
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

bool PackedReader::ReadCount(u64& count)
{
    if (HasError())
    {
        return false;
    }

    u32 narrowed = 0;
    ReadBytes(&narrowed, sizeof(u32));
    if (HasError())
    {
        return false;
    }

    if (narrowed > buffer_view.Len() - offset)
    {
        SetError(String::Format(
            "PackedReader: count {} exceeds remaining bytes ({}).",
            narrowed, buffer_view.Len() - offset
        ));
        return false;
    }

    count = narrowed;
    return true;
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


// PackedFileWriter
PackedFileWriter::PackedFileWriter(Array<u8>& out_buffer, TypeId in_root_type, u64 in_schema_hash)
    : PackedWriter(out_buffer)
    , header_offset(offset)
    , root_type(in_root_type)
    , schema_hash(in_schema_hash)
{
    const PackedFileHeader empty_header{};
    PackedWriter::Bytes(&empty_header, sizeof(empty_header));
}

PackedFileWriter::~PackedFileWriter()
{
    SE_ASSERT(finished || HasError(), "PackedFileWriter: destroyed without Finish().");
}

void PackedFileWriter::Finish()
{
    SE_ASSERT(!finished, "PackedFileWriter::Finish: already finished.");
    finished = true;
    if (HasError())
    {
        return;
    }

    const usize payload_begin = header_offset + sizeof(PackedFileHeader);
    const ArrayView<const u8> payload(buffer.Data() + payload_begin, offset - payload_begin);

    PackedFileHeader header{
        .wire_version = WIRE_VERSION,
        .root_type = root_type.Value(),
        .schema_hash = schema_hash,
        .payload_size = payload.Len(),
        .payload_checksum = ChecksumOf(payload),
    };
    std::memcpy(header.magic, HEADER_MAGIC, sizeof(HEADER_MAGIC));
    std::memcpy(buffer.Data() + header_offset, &header, sizeof(header));
}


// PackedFileReader
PackedFileReader::PackedFileReader(ArrayView<const u8> in_view, TypeId root_type, u64 schema_hash)
    : PackedReader(in_view)
{
    if (buffer_view.Len() < sizeof(PackedFileHeader))
    {
        SetError(String::Format(
            "PackedFileReader: {} bytes is too short for the {}-byte header.", buffer_view.Len(), sizeof(PackedFileHeader)));
        return;
    }

    PackedFileHeader header;
    std::memcpy(&header, buffer_view.Data(), sizeof(header));

    if (std::memcmp(header.magic, HEADER_MAGIC, sizeof(HEADER_MAGIC)) != 0)
    {
        SetError("PackedFileReader: header magic mismatch.");
        return;
    }
    if (header.wire_version != WIRE_VERSION)
    {
        SetError(String::Format("PackedFileReader: unsupported wire version {} (expected {}).", header.wire_version, WIRE_VERSION));
        return;
    }
    if (header.root_type != root_type.Value())
    {
        SetError(String::Format(
            "PackedFileReader: root type id {} does not match the expected type id {}.", header.root_type, root_type.Value()));
        return;
    }
    if (header.schema_hash != schema_hash)
    {
        SetError(String::Format(
            "PackedFileReader: schema hash mismatch (stored {:016x}, expected {:016x}).", header.schema_hash, schema_hash));
        return;
    }

    const ArrayView<const u8> payload = buffer_view.Subview(sizeof(PackedFileHeader));
    if (header.payload_size != payload.Len())
    {
        SetError(String::Format(
            "PackedFileReader: payload size {} does not match the remaining {} bytes.", header.payload_size, payload.Len()));
        return;
    }
    if (header.payload_checksum != ChecksumOf(payload))
    {
        SetError("PackedFileReader: payload checksum mismatch.");
        return;
    }

    offset = sizeof(PackedFileHeader);
}
} // namespace se
