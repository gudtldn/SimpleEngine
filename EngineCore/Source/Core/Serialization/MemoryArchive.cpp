#include "Core/Serialization/MemoryArchive.h"

#include <cstring>

#include "Utility/Debug.h"


namespace se
{
usize MemoryArchive::Tell() const
{
    return offset;
}

void MemoryArchive::Seek(usize pos)
{
    offset = pos;
}

MemoryReader::MemoryReader(const Array<uint8>& in_buffer)
    : MemoryArchive(EArchiveMode::LoadBinary)
    , buffer(in_buffer)
{
}

void MemoryReader::ProcessBytes(void* value, uint64 byte_size)
{
    SE_ASSERT(offset + byte_size <= buffer.Len(), "MemoryReader Overflow! (Offset: {}, Size: {}, BufferLen: {})", offset, byte_size, buffer.Len());

    std::memcpy(value, buffer.Data() + offset, byte_size);
    offset += byte_size;
}

MemoryWriter::MemoryWriter(Array<uint8>& out_buffer)
    : MemoryArchive(EArchiveMode::SaveBinary)
    , buffer(out_buffer)
{
    offset = buffer.Len();
}

void MemoryWriter::ProcessBytes(void* value, uint64 byte_size)
{
    // buffer가 충분히 크지 않으면 확장
    const uint64 required_size = offset + byte_size;
    if (required_size > buffer.Len())
    {
        buffer.ResizeUninitialized(required_size);
    }

    std::memcpy(buffer.Data() + offset, value, byte_size);
    offset += byte_size;
}
}
