#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se
{
/**
 * 바이너리 메모리 직렬화의 기반 클래스
 * 내부 바이트 버퍼에 대한 커서(offset) 관리를 제공합니다.
 */
class SE_CORE_API MemoryArchive : public Archive
{
public:
    virtual ~MemoryArchive() override = default;

    [[nodiscard]] virtual bool IsBinary() const override { return true; }

    /** 현재 커서 위치를 반환합니다. */
    [[nodiscard]] usize Tell() const;

    /** 커서를 특정 위치로 이동합니다. */
    void Seek(usize pos);

protected:
    explicit MemoryArchive(EArchiveMode mode) : Archive(mode) {}
    usize offset = 0;
};


/**
 * 메모리 버퍼에서 데이터를 읽어오는 바이너리 역직렬화 클래스
 */
class SE_CORE_API MemoryReader : public MemoryArchive
{
public:
    explicit MemoryReader(const Array<uint8>& in_buffer);

public:
    virtual void BeginObject() override;
    virtual void EndObject() override;
    virtual void BeginArray(uint64& count) override;
    virtual void EndArray() override;
    virtual void BeginMap(uint64& count) override;
    virtual void EndMap() override;
    virtual void BeginMapKey() override;
    virtual void EndMapKey() override;
    virtual void BeginMapValue() override;
    virtual void EndMapValue() override;

    virtual void SerializeBytes(void* data, uint64 size) override;

protected:
    virtual void HintNextName(const char* name) override;

    virtual void SerializeBool(bool& value) override;
    virtual void SerializeInt8(int8& value) override;
    virtual void SerializeUInt8(uint8& value) override;
    virtual void SerializeInt16(int16& value) override;
    virtual void SerializeUInt16(uint16& value) override;
    virtual void SerializeInt32(int32& value) override;
    virtual void SerializeUInt32(uint32& value) override;
    virtual void SerializeInt64(int64& value) override;
    virtual void SerializeUInt64(uint64& value) override;
    virtual void SerializeFloat(float& value) override;
    virtual void SerializeDouble(double& value) override;

    virtual void SerializeString(String& value) override;
    virtual void SerializeStringName(StringName& value) override;
    virtual void SerializeGuid(Guid& value) override;
    virtual void SerializeTypeId(TypeId& value) override;

private:
    /** Low-level 바이트를 직접 읽습니다. */
    void ReadBytes(void* dest, uint64 byte_size);

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void ReadPrimitive(T& value) { ReadBytes(&value, sizeof(T)); }

private:
    const Array<uint8>& buffer;
};


/**
 * 메모리 버퍼에 데이터를 써넣는 바이너리 직렬화 클래스
 */
class SE_CORE_API MemoryWriter : public MemoryArchive
{
public:
    explicit MemoryWriter(Array<uint8>& out_buffer);

public:
    virtual void BeginObject() override;
    virtual void EndObject() override;
    virtual void BeginArray(uint64& count) override;
    virtual void EndArray() override;
    virtual void BeginMap(uint64& count) override;
    virtual void EndMap() override;
    virtual void BeginMapKey() override;
    virtual void EndMapKey() override;
    virtual void BeginMapValue() override;
    virtual void EndMapValue() override;

    virtual void SerializeBytes(void* data, uint64 size) override;

protected:
    virtual void HintNextName(const char* name) override;

    virtual void SerializeBool(bool& value) override;
    virtual void SerializeInt8(int8& value) override;
    virtual void SerializeUInt8(uint8& value) override;
    virtual void SerializeInt16(int16& value) override;
    virtual void SerializeUInt16(uint16& value) override;
    virtual void SerializeInt32(int32& value) override;
    virtual void SerializeUInt32(uint32& value) override;
    virtual void SerializeInt64(int64& value) override;
    virtual void SerializeUInt64(uint64& value) override;
    virtual void SerializeFloat(float& value) override;
    virtual void SerializeDouble(double& value) override;

    virtual void SerializeString(String& value) override;
    virtual void SerializeStringName(StringName& value) override;
    virtual void SerializeGuid(Guid& value) override;
    virtual void SerializeTypeId(TypeId& value) override;

private:
    /** Low-level 바이트를 직접 씁니다. */
    void WriteBytes(const void* src, uint64 byte_size);

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    void WritePrimitive(T& value) { WriteBytes(&value, sizeof(T)); }

private:
    Array<uint8>& buffer;
};
}  // namespace se
