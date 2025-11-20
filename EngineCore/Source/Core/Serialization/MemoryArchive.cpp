#include "Core/Serialization/MemoryArchive.h"

#include <cstring>

#include "Utility/Debug.h"


namespace se::core
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
    : MemoryArchive(EArchiveMode::Load)
    , buffer(in_buffer)
{
}

void MemoryReader::ProcessRaw(void* value, usize byte_size, const char* name)
{
    SE_ASSERT(offset + byte_size <= buffer.Len(), "MemoryReader Overflow! (Offset: {}, Size: {}, BufferLen: {})", offset, byte_size, buffer.Len());

    std::memcpy(value, buffer.Data() + offset, byte_size);
    offset += byte_size;
}

MemoryWriter::MemoryWriter(Array<uint8>& out_buffer)
    : MemoryArchive(EArchiveMode::Save)
    , buffer(out_buffer)
{
    offset = buffer.Len();
}

void MemoryWriter::ProcessRaw(void* value, usize byte_size, const char* name)
{
    const usize required_size = offset + byte_size;

    // buffer가 충분히 크지 않으면 확장
    if (required_size > buffer.Len())
    {
        buffer.ResizeUninitialized(required_size);
    }

    std::memcpy(buffer.Data() + offset, value, byte_size);
    offset += byte_size;
}
}
