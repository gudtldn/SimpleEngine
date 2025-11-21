#pragma once
#include <type_traits>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"
#include "SimpleEngine/Core/Types/BitFlags.h"
#include "SimpleEngine/Traits/TypeTraits.h"


class Guid;
class StringName;

namespace se
{
template <typename AllocatorType>
class BaseString;

using String = BaseString<core::DefaultAllocator<char>>;
}

namespace se::core
{
class Archive;

/**
 * Archive의 동작 모드
 */
enum class EArchiveMode : uint8
{
    LoadText   = 0, // 00 (Load | Text)
    SaveText   = 1, // 01 (Save | Text)
    LoadBinary = 2, // 10 (Load | Binary)
    SaveBinary = 3, // 11 (Save | Binary)
};

/** 사용자 정의 타입(UDT)에 대한 직렬화 함수의 기본 템플릿 */
template <typename T>
void Serialize([[maybe_unused]] Archive& ar, [[maybe_unused]] T& value)
{
    static_assert(traits::AlwaysFalse<T>,
        "No 'Serialize(Archive&, T&)' function found for this type. "
        "Please define a non-intrusive Serialize function in the same namespace as your type."
    );
}

/**
 * Raw Memory 처리를 위한 래퍼 구조체
 */
class BinaryData
{
public:
    void* data;
    uint64 size;

    static BinaryData FromBytes(void* in_data, uint64 in_byte_size)
    {
        return { in_data, in_byte_size };
    }

    template <typename T>
        requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
    static BinaryData FromItems(T* in_data, uint64 count = 1)
    {
        return { in_data, count * sizeof(T) };
    }

private:
    BinaryData(void* in_data, uint64 in_size)
        : data(in_data), size(in_size)
    {
    }
};

/**
 * 모든 직렬화(Serialization) 작업의 추상 기본 클래스
 */
class SE_CORE_API Archive
{
public:
    virtual ~Archive() = default;

    Archive(const Archive&) = default;
    Archive& operator=(const Archive&) = default;
    Archive(Archive&&) = default;
    Archive& operator=(Archive&&) = default;

public:
    /** 현재 Archive가 로드(읽기) 모드인지 확인합니다. */
    [[nodiscard]] bool IsLoading() const
    {
        // Bit 0이 0이면 Load
        return (static_cast<uint8>(mode) & 1) == 0;
    }

    /** 현재 Archive가 저장(쓰기) 모드인지 확인합니다. */
    [[nodiscard]] bool IsSaving() const
    {
        // Bit 0이 1이면 Save
        return (static_cast<uint8>(mode) & 1) != 0;
    }

    /** 현재 Archive가 Binary 모드인지 확인합니다. */
    [[nodiscard]] bool IsBinary() const
    {
        // Bit 1이 1이면 Binary
        return (static_cast<uint8>(mode) & 2) != 0;
    }

    /** 현재 Archive가 Text 모드인지 확인합니다. */
    [[nodiscard]] bool IsText() const
    {
        // Bit 1이 0이면 Text
        return (static_cast<uint8>(mode) & 2) == 0;
    }

    /**
     * 다음에 직렬화될 값의 이름(Key)에 대한 힌트를 제공합니다.
     * @param name 변수의 이름
     */
    virtual void HintNextName([[maybe_unused]] const char* name) {}

    /** 구조체나 객체의 시작과 끝을 알립니다. */
    virtual void BeginNode() {}
    virtual void EndNode() {}

    /** 배열의 시작과 끝을 알립니다. */
    virtual void BeginArray(uint64& count) { *this << count; }
    virtual void EndArray() {}

protected:
    /**
     * 로우 레벨 바이트 데이터를 처리하는 순수 가상 함수입니다.
     * @param value 데이터가 저장되거나 읽혀질 메모리 주소
     * @param byte_size 처리할 데이터의 바이트 크기
     */
    virtual void ProcessBytes(void* value, uint64 byte_size) = 0;

    template <typename T>
        requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
    void ProcessBytes(T& value)
    {
        ProcessBytes(std::addressof(value), sizeof(T));
    }

public:
    Archive& operator()(const char* name)
    {
        HintNextName(name);
        return *this;
    }

    // POD
    virtual Archive& operator<<(int8& value);
    virtual Archive& operator<<(uint8& value);
    virtual Archive& operator<<(int16& value);
    virtual Archive& operator<<(uint16& value);
    virtual Archive& operator<<(int32& value);
    virtual Archive& operator<<(uint32& value);
    virtual Archive& operator<<(int64& value);
    virtual Archive& operator<<(uint64& value);
    virtual Archive& operator<<(float& value);
    virtual Archive& operator<<(double& value);
    virtual Archive& operator<<(bool& value);

    // Engine Type
    virtual Archive& operator<<(String& value);
    virtual Archive& operator<<(StringName& value);
    virtual Archive& operator<<(Guid& value);

    // Enum
    template <typename EnumType>
        requires std::is_enum_v<EnumType>
    Archive& operator<<(EnumType& value)
    {
        std::underlying_type_t<EnumType>& underlying_value = static_cast<std::underlying_type_t<EnumType>&>(value);
        return *this << underlying_value;
    }

    // BinaryData를 직접 다루는 경우
    friend Archive& operator<<(Archive& ar, const BinaryData& value)
    {
        ar.ProcessBytes(value.data, value.size);
        return ar;
    }

    // 사용자 정의 타입 (UDT)
    template <typename T>
        requires (!std::is_arithmetic_v<T> && !std::is_enum_v<T>)
    friend Archive& operator<<(Archive& ar, T& value)
    {
        ar.BeginNode();
        {
            Serialize(ar, value);
        }
        ar.EndNode();
        return ar;
    }

protected:
    explicit Archive(EArchiveMode in_mode) : mode(in_mode) {}
    EArchiveMode mode;
};
}
