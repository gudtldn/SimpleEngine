#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>


namespace se
{
/** Int/Enum 노드가 다루는 정수 폭 */
enum class EIntWidth : u8
{
    Bits8,
    Bits16,
    Bits32,
    Bits64,
};

/** Float 노드가 다루는 부동소수점 폭 */
enum class EFloatWidth : u8
{
    Bits32,
    Bits64,
};

/** 정수 폭 하나가 차지하는 바이트 수로 변환합니다. */
[[nodiscard]] inline usize ByteSizeOf(EIntWidth width)
{
    switch (width)
    {
        case EIntWidth::Bits8:  return 1;
        case EIntWidth::Bits16: return 2;
        case EIntWidth::Bits32: return 4;
        case EIntWidth::Bits64: return 8;
    }
    SE_UNREACHABLE();
}

/** 정수 또는 문자 타입 T의 크기에 맞는 Int 노드 폭으로 변환합니다. */
template <std::integral T>
[[nodiscard]] constexpr EIntWidth IntWidthOf()
{
    if constexpr (sizeof(T) == 1)      { return EIntWidth::Bits8;  }
    else if constexpr (sizeof(T) == 2) { return EIntWidth::Bits16; }
    else if constexpr (sizeof(T) == 4) { return EIntWidth::Bits32; }
    else if constexpr (sizeof(T) == 8) { return EIntWidth::Bits64; }
    else
    {
        static_assert(traits::AlwaysFalse<T>, "IntWidthOf: unsupported integer size.");
        SE_UNREACHABLE();
    }
}

/** 실수 타입 T의 크기에 맞는 Float 노드 폭으로 변환합니다. */
template <std::floating_point T>
[[nodiscard]] constexpr EFloatWidth FloatWidthOf()
{
    if constexpr (sizeof(T) == 4)      { return EFloatWidth::Bits32; }
    else if constexpr (sizeof(T) == 8) { return EFloatWidth::Bits64; }
    else
    {
        static_assert(traits::AlwaysFalse<T>, "FloatWidthOf: unsupported floating-point size.");
        SE_UNREACHABLE();
    }
}

/** BeginSeq가 나타내는 시퀀스의 원소 순서 보장 여부 */
enum class ESeqOrder : u8
{
    /** 원소 순서에 의미가 있음 (예: Array) */
    Ordered,

    /**
     * 원소 순서에 의미가 없음 (예: Set)
     * @note 텍스트 백엔드는 결정적 출력을 위해 정렬할 수 있습니다.
     */
    Unordered,
};

/**
 * ArchiveWriter/ArchiveReader가 공통으로 갖는 오류 상태와 포맷 질의를 담는 베이스
 */
class SE_CORE_API Archive
{
public:
    virtual ~Archive();

    Archive() = default;
    Archive(const Archive&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive(Archive&&) = delete;
    Archive& operator=(Archive&&) = delete;

public:
    /** 텍스트 포맷인지 확인합니다. */
    [[nodiscard]] virtual bool IsTextFormat() const = 0;

    /** 에러가 발생했는지 확인합니다. 한 번 켜지면 이후 모든 연산은 no-op이어야 합니다. */
    [[nodiscard]] bool HasError() const { return has_error; }

    /** 에러 메시지를 반환합니다. */
    [[nodiscard]] StringView GetError() const { return error_message; }

    /** 에러 상태를 설정합니다. 이미 에러가 있으면 원인 오류를 보존하기 위해 무시합니다. */
    void SetError(String reason);

private:
    bool has_error = false;
    String error_message;
};

/** 직렬화할 값을 노드 단위로 받아 포맷별 표현으로 쓰는 인터페이스 */
class SE_CORE_API ArchiveWriter : public Archive
{
public:
    virtual void Int(i64 value, EIntWidth width, bool is_signed) = 0;
    virtual void Float(f64 value, EFloatWidth width) = 0;
    virtual void Bool(bool value) = 0;
    virtual void Str(StringView value) = 0;
    virtual void Bytes(const void* data, u64 size) = 0;
    virtual void Enum(i64 value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries) = 0;

    virtual void BeginStruct() = 0;
    virtual void Field(StringView name) = 0;
    virtual void EndStruct() = 0;
    virtual void BeginSeq(u64 count, ESeqOrder order) = 0;
    virtual void EndSeq() = 0;
    virtual void BeginMap(u64 count) = 0;
    virtual void BeginMapEntry() = 0;
    virtual void EndMapEntry() = 0;
    virtual void EndMap() = 0;

    /** Optional 노드가 값을 가지고 있는지 여부(field presence)를 씁니다. */
    virtual void Present(bool has_value) = 0;
};

/** 포맷별 표현에서 값을 노드 단위로 읽어 오는 인터페이스 */
class SE_CORE_API ArchiveReader : public Archive
{
public:
    virtual void Int(i64& value, EIntWidth width, bool is_signed) = 0;
    virtual void Float(f64& value, EFloatWidth width) = 0;
    virtual void Bool(bool& value) = 0;
    virtual void Str(String& value) = 0;
    virtual void Bytes(void* data, u64 size) = 0;
    virtual void Enum(i64& value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries) = 0;

    virtual void BeginStruct() = 0;
    [[nodiscard]] virtual bool Field(StringView name) = 0; // 필드가 데이터에 있으면 true
    virtual void EndStruct() = 0;
    virtual void BeginSeq(u64& count) = 0;
    virtual void EndSeq() = 0;
    virtual void BeginMap(u64& count) = 0;
    virtual void BeginMapEntry() = 0;
    virtual void EndMapEntry() = 0;
    virtual void EndMap() = 0;
    virtual void Present(bool& has_value) = 0;
};
} // namespace se
