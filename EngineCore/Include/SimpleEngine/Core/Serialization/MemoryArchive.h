#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se::core
{
class SE_CORE_API MemoryArchive : public Archive
{
public:
    virtual ~MemoryArchive() override = default;

    /** 현재 커서 위치를 반환합니다. */
    [[nodiscard]] usize Tell() const;

    /** 커서를 특정 위치로 이동합니다. */
    void Seek(usize pos);

protected:
    explicit MemoryArchive(EArchiveMode mode) : Archive(mode) {}
    usize offset = 0;
};

class SE_CORE_API MemoryReader : public MemoryArchive
{
public:
    explicit MemoryReader(const Array<uint8>& in_buffer);

protected:
    virtual void ProcessBytes(void* value, usize byte_size) override;

private:
    const Array<uint8>& buffer;
};

class SE_CORE_API MemoryWriter : public MemoryArchive
{
public:
    explicit MemoryWriter(Array<uint8>& out_buffer);

protected:
    virtual void ProcessBytes(void* value, usize byte_size) override;

private:
    Array<uint8>& buffer;
};
}
