#pragma once
/**
 * @file Archive_DEPRECATED.h
 * @deprecated 이 파일은 ArchiveV2.h로 대체될 예정입니다. 새 코드에서는 ArchiveV2.h를 사용하세요.
 */
#include <type_traits>

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se
{
class Guid;
class StringName;
class TypeId;
}  // namespace se

namespace se
{
class Archive_DEPRECATED;

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
void Serialize([[maybe_unused]] Archive_DEPRECATED& ar, [[maybe_unused]] T& value)
{
    static_assert(traits::AlwaysFalse<T>,
        "No 'Serialize(Archive_DEPRECATED&, T&)' function found for this type. "
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
 * @deprecated ArchiveV2로 대체될 예정입니다. 새 코드에서는 ArchiveV2를 사용하세요.
 * @todo Error이 IsSetError(), SetError() 같은 로직 추가. 버전 불일치나, 잘못된 파일 역직렬화 방지용으로
 */
class [[deprecated("Use ArchiveV2 instead.")]] SE_CORE_API Archive_DEPRECATED
{
public:
    virtual ~Archive_DEPRECATED() = default;

    Archive_DEPRECATED(const Archive_DEPRECATED&) = default;
    Archive_DEPRECATED& operator=(const Archive_DEPRECATED&) = default;
    Archive_DEPRECATED(Archive_DEPRECATED&&) = default;
    Archive_DEPRECATED& operator=(Archive_DEPRECATED&&) = default;

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
    Archive_DEPRECATED& operator()(const char* name)
    {
        HintNextName(name);
        return *this;
    }

    Archive_DEPRECATED& operator<<(Archive_DEPRECATED&);

    // POD
    virtual Archive_DEPRECATED& operator<<(int8& value);
    virtual Archive_DEPRECATED& operator<<(uint8& value);
    virtual Archive_DEPRECATED& operator<<(int16& value);
    virtual Archive_DEPRECATED& operator<<(uint16& value);
    virtual Archive_DEPRECATED& operator<<(int32& value);
    virtual Archive_DEPRECATED& operator<<(uint32& value);
    virtual Archive_DEPRECATED& operator<<(int64& value);
    virtual Archive_DEPRECATED& operator<<(uint64& value);
    virtual Archive_DEPRECATED& operator<<(float& value);
    virtual Archive_DEPRECATED& operator<<(double& value);
    virtual Archive_DEPRECATED& operator<<(bool& value);

    // Engine Type
    virtual Archive_DEPRECATED& operator<<(String& value);
    virtual Archive_DEPRECATED& operator<<(StringName& value);
    virtual Archive_DEPRECATED& operator<<(Guid& value);
    virtual Archive_DEPRECATED& operator<<(TypeId& value);

    // Enum
    template <typename EnumType>
        requires std::is_enum_v<EnumType>
    Archive_DEPRECATED& operator<<(EnumType& value)
    {
        using UnderlyingType = std::underlying_type_t<EnumType>;

        UnderlyingType temp_value = static_cast<UnderlyingType>(value);
        *this << temp_value;

        if (IsLoading())
        {
            value = static_cast<EnumType>(temp_value);
        }
        return *this;
    }

    // BinaryData를 직접 다루는 경우
    friend Archive_DEPRECATED& operator<<(Archive_DEPRECATED& ar, const BinaryData& value)
    {
        ar.ProcessBytes(value.data, value.size);
        return ar;
    }

    // 사용자 정의 타입 (UDT)
    template <typename T>
        requires (!std::is_arithmetic_v<T> && !std::is_enum_v<T>)
    friend Archive_DEPRECATED& operator<<(Archive_DEPRECATED& ar, T& value)
    {
        ar.BeginNode();
        {
            Serialize(ar, value);
        }
        ar.EndNode();
        return ar;
    }

protected:
    explicit Archive_DEPRECATED(EArchiveMode in_mode) : mode(in_mode) {}
    EArchiveMode mode;
};
}  // namespace se
