#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#include <type_traits>


namespace se
{
/**
 * 태그 없이 순서에만 의존하는 바이너리 포맷으로 씁니다.
 * @note 파일이나 캐시에 남길 때는 PackedFileWriter를 사용하세요.
 */
class SE_CORE_API PackedWriter : public ArchiveWriter
{
public:
    explicit PackedWriter(Array<u8>& out_buffer);

public:
    [[nodiscard]] virtual bool IsTextFormat() const override;

    virtual void Int(i64 value, EIntWidth width, bool is_signed) override;
    virtual void Float(f64 value, EFloatWidth width) override;
    virtual void Bool(bool value) override;
    virtual void Str(StringView value) override;
    virtual void Bytes(const void* data, u64 size) override;
    virtual void Enum(i64 value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries) override;

    virtual void BeginStruct() override;
    virtual void Field(StringView name) override;
    virtual void EndStruct() override;
    virtual void BeginSeq(u64 count, ESeqOrder order) override;
    virtual void EndSeq() override;
    virtual void BeginMap(u64 count) override;
    virtual void BeginMapEntry() override;
    virtual void EndMapEntry() override;
    virtual void EndMap() override;
    virtual void Present(bool has_value) override;

private:
    void WriteBytes(const void* src, u64 byte_size);
    void WriteCount(u64 count);
    void WriteBoolByte(bool value);

protected:
    Array<u8>& buffer;
    usize offset = 0;
};


/**
 * PackedWriter가 쓴 바이트를 같은 규약으로 되읽습니다.
 */
class SE_CORE_API PackedReader : public ArchiveReader
{
public:
    explicit PackedReader(ArrayView<const u8> in_view);

public:
    [[nodiscard]] virtual bool IsTextFormat() const override;

    virtual void Int(i64& value, EIntWidth width, bool is_signed) override;
    virtual void Float(f64& value, EFloatWidth width) override;
    virtual void Bool(bool& value) override;
    virtual void Str(String& value) override;
    virtual void Bytes(void* data, u64 size) override;
    virtual void Enum(i64& value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries) override;

    virtual void BeginStruct() override;
    [[nodiscard]] virtual bool Field(StringView name) override;
    virtual void EndStruct() override;
    virtual void BeginSeq(u64& count) override;
    virtual void EndSeq() override;
    virtual void BeginMap(u64& count) override;
    virtual void BeginMapEntry() override;
    virtual void EndMapEntry() override;
    virtual void EndMap() override;
    virtual void Present(bool& has_value) override;

private:
    void ReadBytes(void* dest, u64 byte_size);

    /**
     * count 접두를 읽고, 남은 바이트 수를 넘지 않는지 검증합니다.
     * 실패하면 count를 건드리지 않습니다.
     */
    [[nodiscard]] bool ReadCount(u64& count);

    void ReadBoolByte(bool& value);

protected:
    ArrayView<const u8> buffer_view;
    usize offset = 0;
};


/**
 * PackedFileWriter가 payload 앞에 붙이는 헤더
 * 이 구조체의 메모리 표현을 그대로 쓰고 읽으므로(리틀 엔디언) 패딩 없이 둡니다.
 * @note Magic과 WireVersion의 내용은 .cpp에 있습니다.
 */
struct PackedFileHeader
{
    /** 식별 바이트 "SEPK" */
    u8 magic[4] = {};

    /** 헤더 배치와 노드 인코딩의 버전 */
    u32 wire_version = 0;

    u64 root_type = 0;
    u64 schema_hash = 0;
    u64 payload_size = 0;

    /** payload의 XXH3_64bits */
    u64 payload_checksum = 0;
};
static_assert(std::has_unique_object_representations_v<PackedFileHeader>, "PackedFileHeader must not contain padding bytes.");


/**
 * 파일이나 캐시에 남길 Packed 데이터를 씁니다.
 */
class SE_CORE_API PackedFileWriter final : public PackedWriter
{
public:
    PackedFileWriter(Array<u8>& out_buffer, TypeId in_root_type, u64 in_schema_hash);
    virtual ~PackedFileWriter() override;

    /** 헤더를 채웁니다. 헤더 뒤에 쓴 바이트 전체를 payload로 보고 크기와 체크섬을 기록합니다. */
    void Finish();

private:
    usize header_offset = 0;
    TypeId root_type;
    u64 schema_hash = 0;
    bool finished = false;
};


/**
 * PackedFileWriter가 쓴 데이터를 읽습니다. 생성할 때 헤더를 root_type, schema_hash와 대조합니다.
 * 크기, magic, wire 버전, 루트 타입, 스키마 해시, payload 크기, 체크섬 중 하나라도 맞지 않으면 SetError를 호출하고, 이후 읽기는 모두 무시됩니다.
 */
class SE_CORE_API PackedFileReader final : public PackedReader
{
public:
    PackedFileReader(ArrayView<const u8> in_view, TypeId root_type, u64 schema_hash);
};
} // namespace se
