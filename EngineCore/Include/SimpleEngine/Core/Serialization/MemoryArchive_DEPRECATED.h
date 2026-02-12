#pragma once
/**
 * @file MemoryArchive.h
 * @deprecated 이 파일은 MemoryArchiveV2.h로 대체될 예정입니다. 새 코드에서는 MemoryArchiveV2.h를 사용하세요.
 */
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/Archive_DEPRECATED.h"


namespace se
{
class SE_CORE_API MemoryArchive_DEPRECATED : public Archive_DEPRECATED
{
public:
    virtual ~MemoryArchive_DEPRECATED() override = default;

    /** 현재 커서 위치를 반환합니다. */
    [[nodiscard]] usize Tell() const;

    /** 커서를 특정 위치로 이동합니다. */
    void Seek(usize pos);

protected:
    explicit MemoryArchive_DEPRECATED(EArchiveMode mode) : Archive_DEPRECATED(mode) {}
    usize offset = 0;
};

class SE_CORE_API MemoryReader_DEPRECATED : public MemoryArchive_DEPRECATED
{
public:
    explicit MemoryReader_DEPRECATED(const Array<uint8>& in_buffer);

protected:
    virtual void ProcessBytes(void* value, uint64 byte_size) override;

private:
    const Array<uint8>& buffer;
};

class SE_CORE_API MemoryWriter_DEPRECATED : public MemoryArchive_DEPRECATED
{
public:
    explicit MemoryWriter_DEPRECATED(Array<uint8>& out_buffer);

protected:
    virtual void ProcessBytes(void* value, uint64 byte_size) override;

private:
    Array<uint8>& buffer;
};
}
