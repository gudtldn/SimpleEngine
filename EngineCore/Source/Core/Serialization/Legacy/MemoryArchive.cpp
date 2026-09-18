#include "SimpleEngine/Core/Serialization/Legacy/MemoryArchive.h"

#include "SimpleEngine/Core/Container/String.h"
#include "../../../../Include/SimpleEngine/Core/Reflection/Legacy/TypeId.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
// MemoryArchive_v1 (공통 기반)
usize MemoryArchive_v1::Tell() const
{
    return offset;
}

void MemoryArchive_v1::Seek(usize pos)
{
    SE_ENSURE(pos <= offset, "MemoryArchive_v1::Seek - Seeking beyond written range. (pos: {}, written: {})", pos, offset);
    offset = pos;
}


// MemoryReader_v1
MemoryReader_v1::MemoryReader_v1(ArrayView<const u8> in_view)
    : MemoryArchive_v1(EArchiveMode_v1::Load)
    , buffer_view(in_view)
{
}

void MemoryReader_v1::BeginObject() {}
void MemoryReader_v1::EndObject() {}
void MemoryReader_v1::BeginArray(u64& count)
{
    ReadPrimitive(count);
}

void MemoryReader_v1::EndArray() {}
void MemoryReader_v1::BeginMap(u64& count)
{
    ReadPrimitive(count);
}
void MemoryReader_v1::EndMap() {}
void MemoryReader_v1::BeginMapKey() {}
void MemoryReader_v1::EndMapKey() {}
void MemoryReader_v1::BeginMapValue() {}
void MemoryReader_v1::EndMapValue() {}

void MemoryReader_v1::SerializeBytes(void* data, u64 size)
{
    ReadBytes(data, size);
}

// 이름 힌트 (바이너리에서는 무시)
void MemoryReader_v1::HintNextName([[maybe_unused]] StringView name) {}

void MemoryReader_v1::SerializeBool(bool& value)
{
    u8 temp = 0;
    ReadPrimitive(temp);
    value = temp != 0;
}

void MemoryReader_v1::SerializeInt8(i8& value)      { ReadPrimitive(value); }
void MemoryReader_v1::SerializeUInt8(u8& value)    { ReadPrimitive(value); }
void MemoryReader_v1::SerializeInt16(i16& value)    { ReadPrimitive(value); }
void MemoryReader_v1::SerializeUInt16(u16& value)  { ReadPrimitive(value); }
void MemoryReader_v1::SerializeInt32(i32& value)    { ReadPrimitive(value); }
void MemoryReader_v1::SerializeUInt32(u32& value)  { ReadPrimitive(value); }
void MemoryReader_v1::SerializeInt64(i64& value)    { ReadPrimitive(value); }
void MemoryReader_v1::SerializeUInt64(u64& value)  { ReadPrimitive(value); }
void MemoryReader_v1::SerializeFloat(f32& value)    { ReadPrimitive(value); }
void MemoryReader_v1::SerializeDouble(f64& value)  { ReadPrimitive(value); }

void MemoryReader_v1::SerializeString(String& value)
{
    u64 length = 0;
    ReadPrimitive(length);
    value.ResizeUninitialized(length);
    ReadBytes(value.Data(), length);
}

void MemoryReader_v1::SerializeStringName(StringName& value)
{
    String temp;
    SerializeString(temp);
    value = temp;
}

void MemoryReader_v1::SerializeGuid(Guid& value)
{
    ReadBytes(&value, sizeof(Guid));
}

void MemoryReader_v1::SerializeTypeId(TypeId_v1& value)
{
    u64 hash = 0;
    ReadPrimitive(hash);
    value = TypeId_v1::FromHash(hash);
    if (!SE_ENSURE(value.IsValid(), "MemoryReader_v1::SerializeTypeId - Failed to resolve TypeId from hash: {}. The class might be deleted or renamed.", hash))
    {
        SetError(String::Format("MemoryReader_v1: Failed to resolve TypeId from hash: {}.", hash));
    }
}

void MemoryReader_v1::ReadBytes(void* dest, u64 byte_size)
{
    if (!SE_ENSURE( // NOLINT(*-simplify-boolean-expr)
        offset + byte_size <= buffer_view.Len(),
        "MemoryReader_v1::ReadBytes - Buffer overflow! (Offset: {}, Size: {}, BufferLen: {})", offset, byte_size, buffer_view.Len()
    ))
    {
        SetError(String::Format("MemoryReader_v1: Buffer overflow! (Offset: {}, Size: {}, BufferLen: {})", offset, byte_size, buffer_view.Len()));

        // 릴리스에서 오버플로우 시 남은 만큼만 읽고 나머지는 0으로 채움
        const u64 readable = (offset < buffer_view.Len()) ? buffer_view.Len() - offset : 0;
        if (readable > 0)
        {
            std::memcpy(dest, buffer_view.Data() + offset, readable);
        }
        std::memset(static_cast<u8*>(dest) + readable, 0, byte_size - readable);
        offset = buffer_view.Len();
        return;
    }

    std::memcpy(dest, buffer_view.Data() + offset, byte_size);
    offset += byte_size;
}


// MemoryWriter_v1
MemoryWriter_v1::MemoryWriter_v1(Array<u8>& out_buffer)
    : MemoryArchive_v1(EArchiveMode_v1::Save)
    , buffer(out_buffer)
{
    offset = buffer.Len();
}

void MemoryWriter_v1::BeginObject() {}
void MemoryWriter_v1::EndObject() {}
void MemoryWriter_v1::BeginArray(u64& count)
{
    WritePrimitive(count);
}
void MemoryWriter_v1::EndArray() {}
void MemoryWriter_v1::BeginMap(u64& count)
{
    WritePrimitive(count);
}
void MemoryWriter_v1::EndMap() {}
void MemoryWriter_v1::BeginMapKey() {}
void MemoryWriter_v1::EndMapKey() {}
void MemoryWriter_v1::BeginMapValue() {}
void MemoryWriter_v1::EndMapValue() {}

void MemoryWriter_v1::SerializeBytes(void* data, u64 size)
{
    WriteBytes(data, size);
}

// 이름 힌트 (바이너리에서는 무시)
void MemoryWriter_v1::HintNextName([[maybe_unused]] StringView name) {}

void MemoryWriter_v1::SerializeBool(bool& value)
{
    u8 temp = value ? 1 : 0;
    WritePrimitive(temp);
}

void MemoryWriter_v1::SerializeInt8(i8& value)      { WritePrimitive(value); }
void MemoryWriter_v1::SerializeUInt8(u8& value)    { WritePrimitive(value); }
void MemoryWriter_v1::SerializeInt16(i16& value)    { WritePrimitive(value); }
void MemoryWriter_v1::SerializeUInt16(u16& value)  { WritePrimitive(value); }
void MemoryWriter_v1::SerializeInt32(i32& value)    { WritePrimitive(value); }
void MemoryWriter_v1::SerializeUInt32(u32& value)  { WritePrimitive(value); }
void MemoryWriter_v1::SerializeInt64(i64& value)    { WritePrimitive(value); }
void MemoryWriter_v1::SerializeUInt64(u64& value)  { WritePrimitive(value); }
void MemoryWriter_v1::SerializeFloat(f32& value)    { WritePrimitive(value); }
void MemoryWriter_v1::SerializeDouble(f64& value)  { WritePrimitive(value); }

void MemoryWriter_v1::SerializeString(String& value)
{
    u64 length = value.ByteLen();
    WritePrimitive(length);
    WriteBytes(value.Data(), length);
}

void MemoryWriter_v1::SerializeStringName(StringName& value)
{
    String temp = value.ToString();
    SerializeString(temp);
}

void MemoryWriter_v1::SerializeGuid(Guid& value)
{
    WriteBytes(&value, sizeof(Guid));
}

void MemoryWriter_v1::SerializeTypeId(TypeId_v1& value)
{
    u64 hash = 0;
    if (SE_ENSURE(value.IsValid(), "Attempting to save invalid TypeId via Binary!"))
    {
        hash = value.GetHash();
    }
    else
    {
        SetError("MemoryWriter_v1: Attempting to save invalid TypeId.");
    }
    WritePrimitive(hash);
}

void MemoryWriter_v1::WriteBytes(const void* src, u64 byte_size)
{
    const u64 required_size = offset + byte_size;
    if (required_size > buffer.Len())
    {
        buffer.ResizeUninitialized(required_size);
    }
    std::memcpy(buffer.Data() + offset, src, byte_size);
    offset += byte_size;
}
} // namespace se
