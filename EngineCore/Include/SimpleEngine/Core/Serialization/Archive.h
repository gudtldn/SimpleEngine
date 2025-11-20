#pragma once
#include <type_traits>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"
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
    Load,
    Save
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
 * 모든 직렬화(Serialization) 작업의 추상 기본 클래스
 */
class SE_CORE_API Archive
{
public:
    virtual ~Archive() = default;

    Archive(const Archive&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive(Archive&&) = delete;
    Archive& operator=(Archive&&) = delete;

public:
    /** 현재 Archive가 로드(읽기) 모드인지 확인합니다. */
    [[nodiscard]] bool IsLoading() const { return mode == EArchiveMode::Load; }

    /** 현재 Archive가 저장(쓰기) 모드인지 확인합니다. */
    [[nodiscard]] bool IsSaving() const { return mode == EArchiveMode::Save; }

    /**
     * 다음에 직렬화될 값의 이름(Key)에 대한 힌트를 제공합니다.
     * @param name 변수의 이름
     */
    virtual void HintNextName([[maybe_unused]] const char* name) {}

    /** 구조체나 객체의 시작(Node 진입)을 알립니다. */
    virtual void BeginNode() {}

    /** 구조체나 객체의 끝(Node 탈출)을 알립니다. */
    virtual void EndNode() {}

protected:
    /**
     * 로우 레벨 바이트 데이터를 처리하는 순수 가상 함수입니다.
     * @param value 데이터가 저장되거나 읽혀질 메모리 주소
     * @param byte_size 처리할 데이터의 바이트 크기
     */
    virtual void ProcessBytes(void* value, usize byte_size) = 0;

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
