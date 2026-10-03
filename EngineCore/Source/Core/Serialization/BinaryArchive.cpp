#include "SimpleEngine/Core/Serialization/BinaryArchive.h"

#include "SimpleEngine/Core/Container/String.h"

#include <xxhash.h>

#include <bit>
#include <cstring>
#include <limits>


namespace se
{
// Int/Float를 바이트 순서 변환 없이 memcpy로 쓰고 읽으므로 리틀 엔디언 플랫폼만 지원합니다.
// 나중에 빅엔디언을 지원하려면 Int/Float의 쓰기와 읽기에 std::byteswap을 넣어야 합니다.
static_assert(std::endian::native == std::endian::little, "BinaryArchive only supports little-endian platforms.");

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


// BinaryWriter
BinaryWriter::BinaryWriter(Array<u8>& out_buffer)
    : buffer(out_buffer)
{
    offset = buffer.Len();
}

bool BinaryWriter::IsTextFormat() const
{
    return false;
}

bool BinaryWriter::SupportsRawElements() const
{
    return true;
}

void BinaryWriter::Int(i64 value, EIntWidth width, [[maybe_unused]] bool is_signed)
{
    // 리틀 엔디안이므로 하위 n바이트만 잘라 쓰면 됨
    WriteBytes(&value, serde::ByteSizeOf(width));
}

void BinaryWriter::Float(f64 value, EFloatWidth width)
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

void BinaryWriter::Bool(bool value)
{
    WriteBoolByte(value);
}

void BinaryWriter::Str(StringView value)
{
    WriteCount(value.ByteLen());
    WriteBytes(value.Data(), value.ByteLen());
}

void BinaryWriter::Bytes(const void* data, u64 size)
{
    WriteBytes(data, size);
}

void BinaryWriter::Enum(i64 value, EIntWidth width, bool is_signed, [[maybe_unused]] ArrayView<const EnumEntry> entries)
{
    // 바이너리에서는 entries(이름 테이블)를 쓰지 않고 Int와 동일하게 취급
    Int(value, width, is_signed);
}

void BinaryWriter::BeginStruct() {}
void BinaryWriter::Field([[maybe_unused]] StringView name) {}
void BinaryWriter::EndStruct() {}

void BinaryWriter::BeginSeq(u64 count, [[maybe_unused]] ESeqOrder order)
{
    // Binary는 순서를 되살릴 필요가 없으므로 ESeqOrder를 무시
    WriteCount(count);
}

void BinaryWriter::EndSeq() {}

void BinaryWriter::BeginMap(u64 count)
{
    WriteCount(count);
}

void BinaryWriter::BeginMapEntry() {}
void BinaryWriter::EndMapEntry() {}
void BinaryWriter::EndMap() {}

void BinaryWriter::Present(bool has_value)
{
    WriteBoolByte(has_value);
}

void BinaryWriter::RawElements(const void* data, u64 size)
{
    WriteBytes(data, size);
}

void BinaryWriter::BeginSection()
{
    if (HasError())
    {
        return;
    }

    // 길이는 내용을 다 써야 알 수 있으므로 자리만 잡고 EndSection에서 채움
    open_sections.Push(offset);
    constexpr u64 placeholder = 0;
    WriteBytes(&placeholder, sizeof(u64));
}

void BinaryWriter::EndSection()
{
    if (HasError())
    {
        return;
    }

    const auto length_offset = open_sections.Pop();
    if (!length_offset)
    {
        SetError("BinaryWriter: EndSection does not match an open section.");
        return;
    }
    const u64 length = offset - (*length_offset + sizeof(u64));
    std::memcpy(buffer.Data() + *length_offset, &length, sizeof(u64));
}

void BinaryWriter::WriteBytes(const void* src, u64 byte_size)
{
    // 빈 컨테이너의 데이터 포인터는 nullptr일 수 있고, memcpy에 nullptr를 넘기면 크기가 0이어도 정의되지 않은 동작
    if (HasError() || byte_size == 0)
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

void BinaryWriter::WriteCount(u64 count)
{
    if (HasError())
    {
        return;
    }
    if (count > std::numeric_limits<u32>::max())
    {
        SetError(String::Format("BinaryWriter: count {} exceeds u32 range.", count));
        return;
    }
    const u32 narrowed = static_cast<u32>(count);
    WriteBytes(&narrowed, sizeof(u32));
}

void BinaryWriter::WriteBoolByte(bool value)
{
    const u8 byte = value ? 1 : 0;
    WriteBytes(&byte, 1);
}


// BinaryReader
BinaryReader::BinaryReader(ArrayView<const u8> in_view)
    : buffer_view(in_view)
{
}

bool BinaryReader::IsTextFormat() const
{
    return false;
}

bool BinaryReader::SupportsRawElements() const
{
    return true;
}

void BinaryReader::Int(i64& value, EIntWidth width, bool is_signed)
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

void BinaryReader::Float(f64& value, EFloatWidth width)
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

void BinaryReader::Bool(bool& value)
{
    ReadBoolByte(value);
}

void BinaryReader::Str(String& value)
{
    u64 length = 0;
    if (!ReadCount(length))
    {
        return;
    }

    value.ResizeUninitialized(length);
    ReadBytes(value.Data(), length);
}

void BinaryReader::Bytes(void* data, u64 size)
{
    ReadBytes(data, size);
}

void BinaryReader::Enum(i64& value, EIntWidth width, bool is_signed, [[maybe_unused]] ArrayView<const EnumEntry> entries)
{
    Int(value, width, is_signed);
}

void BinaryReader::BeginStruct() {}

bool BinaryReader::Field([[maybe_unused]] StringView name)
{
    return true;
}

void BinaryReader::EndStruct() {}

void BinaryReader::BeginSeq(u64& count)
{
    if (u64 result = 0; ReadCount(result))
    {
        count = result;
    }
}

void BinaryReader::EndSeq() {}

void BinaryReader::BeginMap(u64& count)
{
    if (u64 result = 0; ReadCount(result))
    {
        count = result;
    }
}

void BinaryReader::BeginMapEntry() {}
void BinaryReader::EndMapEntry() {}
void BinaryReader::EndMap() {}

void BinaryReader::Present(bool& has_value)
{
    ReadBoolByte(has_value);
}

void BinaryReader::RawElements(void* data, u64 size)
{
    ReadBytes(data, size);
}

void BinaryReader::BeginSection()
{
    if (u64 length = 0; ReadSectionLength(length))
    {
        section_ends.Push(offset + length);
    }
}

void BinaryReader::EndSection()
{
    if (HasError())
    {
        return;
    }

    const auto end = section_ends.Pop();
    if (!end)
    {
        SetError("BinaryReader: EndSection does not match an open section.");
        return;
    }

    // 덜 읽거나 더 읽었으면 쓴 타입과 읽는 타입이 다르거나 데이터가 손상된 것
    if (offset != *end)
    {
        SetError(String::Format("BinaryReader: section ends at offset {}, but reading stopped at offset {}.", *end, offset));
    }
}

void BinaryReader::SkipSection()
{
    if (u64 length = 0; ReadSectionLength(length))
    {
        offset += length;
    }
}

void BinaryReader::Rewind()
{
    offset = start_offset;
    section_ends.Clear();
}

void BinaryReader::ReadBytes(void* dest, u64 byte_size)
{
    // WriteBytes와 같은 이유로 0바이트는 memcpy를 부르지 않음
    if (HasError() || byte_size == 0)
    {
        return;
    }

    if (byte_size > buffer_view.Len() - offset)
    {
        SetError(String::Format(
            "BinaryReader: buffer overflow. (offset: {}, size: {}, buffer_len: {})",
            offset, byte_size, buffer_view.Len()
        ));
        return;
    }
    std::memcpy(dest, buffer_view.Data() + offset, byte_size);
    offset += byte_size;
}

bool BinaryReader::ReadCount(u64& count)
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
            "BinaryReader: count {} exceeds remaining bytes ({}).",
            narrowed, buffer_view.Len() - offset
        ));
        return false;
    }

    count = narrowed;
    return true;
}

bool BinaryReader::ReadSectionLength(u64& length)
{
    if (HasError())
    {
        return false;
    }

    u64 result = 0;
    ReadBytes(&result, sizeof(u64));
    if (HasError())
    {
        return false;
    }

    if (result > buffer_view.Len() - offset)
    {
        SetError(String::Format(
            "BinaryReader: section length {} exceeds remaining bytes ({}).",
            result, buffer_view.Len() - offset
        ));
        return false;
    }

    length = result;
    return true;
}

void BinaryReader::ReadBoolByte(bool& value)
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


// BinaryFileWriter
BinaryFileWriter::BinaryFileWriter(Array<u8>& out_buffer, TypeId in_root_type, u64 in_schema_hash)
    : BinaryWriter(out_buffer)
    , header_offset(offset)
    , root_type(in_root_type)
    , schema_hash(in_schema_hash)
{
    const BinaryFileHeader empty_header{};
    BinaryWriter::Bytes(&empty_header, sizeof(empty_header));
}

BinaryFileWriter::~BinaryFileWriter()
{
    SE_ASSERT(finished || HasError(), "BinaryFileWriter: destroyed without Finish().");
}

void BinaryFileWriter::Finish()
{
    SE_ASSERT(!finished, "BinaryFileWriter::Finish: already finished.");
    finished = true;
    if (HasError())
    {
        return;
    }

    const usize payload_begin = header_offset + sizeof(BinaryFileHeader);
    const ArrayView<const u8> payload(buffer.Data() + payload_begin, offset - payload_begin);

    BinaryFileHeader header{
        .wire_version = WIRE_VERSION,
        .root_type = root_type.Value(),
        .schema_hash = schema_hash,
        .payload_size = payload.Len(),
        .payload_checksum = ChecksumOf(payload),
    };
    std::memcpy(header.magic, HEADER_MAGIC, sizeof(HEADER_MAGIC));
    std::memcpy(buffer.Data() + header_offset, &header, sizeof(header));
}


// BinaryFileReader
BinaryFileReader::BinaryFileReader(ArrayView<const u8> in_view, TypeId root_type, u64 schema_hash)
    : BinaryReader(in_view)
{
    if (buffer_view.Len() < sizeof(BinaryFileHeader))
    {
        SetError(String::Format(
            "BinaryFileReader: {} bytes is too short for the {}-byte header.", buffer_view.Len(), sizeof(BinaryFileHeader)));
        return;
    }

    BinaryFileHeader header;
    std::memcpy(&header, buffer_view.Data(), sizeof(header));

    if (std::memcmp(header.magic, HEADER_MAGIC, sizeof(HEADER_MAGIC)) != 0)
    {
        SetError("BinaryFileReader: header magic mismatch.");
        return;
    }
    if (header.wire_version != WIRE_VERSION)
    {
        SetError(String::Format("BinaryFileReader: unsupported wire version {} (expected {}).", header.wire_version, WIRE_VERSION));
        return;
    }
    if (header.root_type != root_type.Value())
    {
        SetError(String::Format(
            "BinaryFileReader: root type id {} does not match the expected type id {}.", header.root_type, root_type.Value()));
        return;
    }
    if (header.schema_hash != schema_hash)
    {
        SetError(String::Format(
            "BinaryFileReader: schema hash mismatch (stored {:016x}, expected {:016x}).", header.schema_hash, schema_hash));
        return;
    }

    const ArrayView<const u8> payload = buffer_view.Subview(sizeof(BinaryFileHeader));
    if (header.payload_size != payload.Len())
    {
        SetError(String::Format(
            "BinaryFileReader: payload size {} does not match the remaining {} bytes.", header.payload_size, payload.Len()));
        return;
    }
    if (header.payload_checksum != ChecksumOf(payload))
    {
        SetError("BinaryFileReader: payload checksum mismatch.");
        return;
    }

    offset = sizeof(BinaryFileHeader);
    start_offset = offset;
}
} // namespace se
