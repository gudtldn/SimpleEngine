#include "SimpleEngine/Core/Serialization/MemoryArchive.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
// MemoryArchive (공통 기반)
usize MemoryArchive::Tell() const
{
    return offset;
}

void MemoryArchive::Seek(usize pos)
{
    SE_ENSURE(pos <= offset, "MemoryArchive::Seek - Seeking beyond written range. (pos: {}, written: {})", pos, offset);
    offset = pos;
}


// MemoryReader
MemoryReader::MemoryReader(ArrayView<const uint8> in_view)
    : MemoryArchive(EArchiveMode::Load)
    , buffer_view(in_view)
{
}

void MemoryReader::BeginObject() {}
void MemoryReader::EndObject() {}
void MemoryReader::BeginArray(uint64& count)
{
    ReadPrimitive(count);
}

void MemoryReader::EndArray() {}
void MemoryReader::BeginMap(uint64& count)
{
    ReadPrimitive(count);
}
void MemoryReader::EndMap() {}
void MemoryReader::BeginMapKey() {}
void MemoryReader::EndMapKey() {}
void MemoryReader::BeginMapValue() {}
void MemoryReader::EndMapValue() {}

void MemoryReader::SerializeBytes(void* data, uint64 size)
{
    ReadBytes(data, size);
}

// 이름 힌트 (바이너리에서는 무시)
void MemoryReader::HintNextName([[maybe_unused]] StringView name) {}

void MemoryReader::SerializeBool(bool& value)
{
    uint8 temp = 0;
    ReadPrimitive(temp);
    value = temp != 0;
}

void MemoryReader::SerializeInt8(int8& value)      { ReadPrimitive(value); }
void MemoryReader::SerializeUInt8(uint8& value)    { ReadPrimitive(value); }
void MemoryReader::SerializeInt16(int16& value)    { ReadPrimitive(value); }
void MemoryReader::SerializeUInt16(uint16& value)  { ReadPrimitive(value); }
void MemoryReader::SerializeInt32(int32& value)    { ReadPrimitive(value); }
void MemoryReader::SerializeUInt32(uint32& value)  { ReadPrimitive(value); }
void MemoryReader::SerializeInt64(int64& value)    { ReadPrimitive(value); }
void MemoryReader::SerializeUInt64(uint64& value)  { ReadPrimitive(value); }
void MemoryReader::SerializeFloat(float& value)    { ReadPrimitive(value); }
void MemoryReader::SerializeDouble(double& value)  { ReadPrimitive(value); }

void MemoryReader::SerializeString(String& value)
{
    uint64 length = 0;
    ReadPrimitive(length);
    value.ResizeForOverwrite(length);
    ReadBytes(value.Data(), length);
}

void MemoryReader::SerializeStringName(StringName& value)
{
    String temp;
    SerializeString(temp);
    value = temp;
}

void MemoryReader::SerializeGuid(Guid& value)
{
    ReadBytes(&value, sizeof(Guid));
}

void MemoryReader::SerializeTypeId(TypeId& value)
{
    uint64 hash = 0;
    ReadPrimitive(hash);
    value = TypeId::FromHash(hash);
    SE_ENSURE(value.IsValid(), "MemoryReader::SerializeTypeId - Failed to resolve TypeId from hash: {}. The class might be deleted or renamed.", hash);
}

void MemoryReader::ReadBytes(void* dest, uint64 byte_size)
{
    if (!SE_ENSURE( // NOLINT(*-simplify-boolean-expr)
        offset + byte_size <= buffer_view.Len(),
        "MemoryReader::ReadBytes - Buffer overflow! (Offset: {}, Size: {}, BufferLen: {})", offset, byte_size, buffer_view.Len()
    ))
    {
        // 릴리스에서 오버플로우 시 남은 만큼만 읽고 나머지는 0으로 채움
        const uint64 readable = (offset < buffer_view.Len()) ? buffer_view.Len() - offset : 0;
        if (readable > 0)
        {
            std::memcpy(dest, buffer_view.Data() + offset, readable);
        }
        std::memset(static_cast<uint8*>(dest) + readable, 0, byte_size - readable);
        offset = buffer_view.Len();
        return;
    }

    std::memcpy(dest, buffer_view.Data() + offset, byte_size);
    offset += byte_size;
}


// MemoryWriter
MemoryWriter::MemoryWriter(Array<uint8>& out_buffer)
    : MemoryArchive(EArchiveMode::Save)
    , buffer(out_buffer)
{
    offset = buffer.Len();
}

void MemoryWriter::BeginObject() {}
void MemoryWriter::EndObject() {}
void MemoryWriter::BeginArray(uint64& count)
{
    WritePrimitive(count);
}
void MemoryWriter::EndArray() {}
void MemoryWriter::BeginMap(uint64& count)
{
    WritePrimitive(count);
}
void MemoryWriter::EndMap() {}
void MemoryWriter::BeginMapKey() {}
void MemoryWriter::EndMapKey() {}
void MemoryWriter::BeginMapValue() {}
void MemoryWriter::EndMapValue() {}

void MemoryWriter::SerializeBytes(void* data, uint64 size)
{
    WriteBytes(data, size);
}

// 이름 힌트 (바이너리에서는 무시)
void MemoryWriter::HintNextName([[maybe_unused]] StringView name) {}

void MemoryWriter::SerializeBool(bool& value)
{
    uint8 temp = value ? 1 : 0;
    WritePrimitive(temp);
}

void MemoryWriter::SerializeInt8(int8& value)      { WritePrimitive(value); }
void MemoryWriter::SerializeUInt8(uint8& value)    { WritePrimitive(value); }
void MemoryWriter::SerializeInt16(int16& value)    { WritePrimitive(value); }
void MemoryWriter::SerializeUInt16(uint16& value)  { WritePrimitive(value); }
void MemoryWriter::SerializeInt32(int32& value)    { WritePrimitive(value); }
void MemoryWriter::SerializeUInt32(uint32& value)  { WritePrimitive(value); }
void MemoryWriter::SerializeInt64(int64& value)    { WritePrimitive(value); }
void MemoryWriter::SerializeUInt64(uint64& value)  { WritePrimitive(value); }
void MemoryWriter::SerializeFloat(float& value)    { WritePrimitive(value); }
void MemoryWriter::SerializeDouble(double& value)  { WritePrimitive(value); }

void MemoryWriter::SerializeString(String& value)
{
    uint64 length = value.ByteLen();
    WritePrimitive(length);
    WriteBytes(value.Data(), length);
}

void MemoryWriter::SerializeStringName(StringName& value)
{
    String temp = value.ToString();
    SerializeString(temp);
}

void MemoryWriter::SerializeGuid(Guid& value)
{
    WriteBytes(&value, sizeof(Guid));
}

void MemoryWriter::SerializeTypeId(TypeId& value)
{
    uint64 hash = 0;
    if (SE_ENSURE(value.IsValid(), "Attempting to save invalid TypeId via Binary!"))
    {
        hash = value.GetHash();
    }
    WritePrimitive(hash);
}

void MemoryWriter::WriteBytes(const void* src, uint64 byte_size)
{
    const uint64 required_size = offset + byte_size;
    if (required_size > buffer.Len())
    {
        buffer.ResizeUninitialized(required_size);
    }
    std::memcpy(buffer.Data() + offset, src, byte_size);
    offset += byte_size;
}
}  // namespace se
