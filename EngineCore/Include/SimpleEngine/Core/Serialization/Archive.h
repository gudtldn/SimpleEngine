#pragma once
#include <concepts>
#include <memory>
#include <type_traits>

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Traits.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Traits/ContainerTraits.h"
#include "SimpleEngine/Traits/SerializationTraits.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se
{
// Forward declarations
class Archive;
class Guid;
class StringName;

/** 리플렉션 기반 자동 직렬화 (TypeId 조회 후 프로퍼티 순회) */
SE_CORE_API void AutoSerialize(Archive& ar, const TypeId& type_id, void* instance);

/**
 * Archive의 동작 모드
 */
enum class EArchiveMode : uint8
{
    Load = 0,
    Save = 1,
};

/**
 * Raw Memory 처리를 위한 Wrapper 구조체
 */
class BinaryBlob
{
public:
    void* data;
    uint64 size;

    static BinaryBlob FromBytes(void* in_data, uint64 in_byte_size)
    {
        return { in_data, in_byte_size };
    }

    template <typename T>
        requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
    static BinaryBlob FromItems(T* in_data, uint64 count = 1)
    {
        return { in_data, count * sizeof(T) };
    }

private:
    BinaryBlob(void* in_data, uint64 in_size)
        : data(in_data), size(in_size)
    {
    }
};

/**
 * 모든 직렬화(Serialization) 시스템의 추상 기본 클래스
 */
class SE_CORE_API Archive
{
public:
    virtual ~Archive();

    // 복사 금지 & 이동만 허용
    Archive(const Archive&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive(Archive&&) noexcept;
    Archive& operator=(Archive&&) noexcept;

public:
    /** 현재 Archive가 로드(읽기) 모드인지 확인합니다. */
    [[nodiscard]] bool IsLoading() const { return mode == EArchiveMode::Load; }

    /** 현재 Archive가 저장(쓰기) 모드인지 확인합니다. */
    [[nodiscard]] bool IsSaving() const { return mode == EArchiveMode::Save; }

    /** 현재 Archive가 Binary 포맷인지 확인합니다. */
    [[nodiscard]] virtual bool IsBinary() const = 0;

    /** 현재 Archive가 Text 포맷인지 확인합니다. */
    [[nodiscard]] bool IsText() const { return !IsBinary(); }

public:
    /** 에러가 발생했는지 확인합니다. */
    [[nodiscard]] bool HasError() const { return error_message != nullptr; }

    /** 에러 메시지를 반환합니다. */
    [[nodiscard]] StringView GetError() const { return *error_message; }

    /** 에러 상태를 설정합니다. */
    void SetError(StringView reason);

    /** 에러 상태를 초기화합니다. */
    void ClearError();

public:
    /**
     * 다음에 직렬화될 값의 이름(Key)에 대한 힌트를 제공합니다.
     * 텍스트 포맷에서는 키 이름으로 사용되고, 바이너리에서는 무시됩니다.
     * @param name 변수의 이름
     * @return 체이닝을 위한 자기 자신 참조
     */
    Archive& operator()(StringView name)
    {
        HintNextName(name);
        return *this;
    }

    /**
     * 모든 타입의 단일 직렬화 진입점.
     * 타입에 따라 적절한 가상 함수를 호출합니다.
     */
    template <typename T>
    Archive& operator<<(T& value);

    /**
     * 상수(const) 객체를 위한 저장(Save) 전용 진입점.
     * 복사 오버헤드 방지를 위해 내부에서 const_cast<T&>후 전달합니다. Load 모드에서 호출 시 Assert를 발생시킵니다.
     */
    template <typename T>
    Archive& operator<<(const T& value);

    /** BinaryBlob을 직접 다루는 경우 */
    friend Archive& operator<<(Archive& ar, const BinaryBlob& blob)
    {
        ar.SerializeBytes(blob.data, blob.size);
        return ar;
    }

public:
    /** 구조체나 객체의 시작을 알립니다. */
    virtual void BeginObject() = 0;
    /** 구조체나 객체의 끝을 알립니다. */
    virtual void EndObject() = 0;

    /**
     * 배열의 시작을 알립니다.
     * @param count Save 시 입력값(배열 크기), Load 시 출력값(읽어올 크기)
     */
    virtual void BeginArray(uint64& count) = 0;
    /** 배열의 끝을 알립니다. */
    virtual void EndArray() = 0;

    /**
     * Map(Key-Value) 구조의 시작을 알립니다.
     * 텍스트 포맷에서는 table/object로 표현될 수 있습니다.
     * @param count Save 시 입력값(엔트리 수), Load 시 출력값
     */
    virtual void BeginMap(uint64& count) = 0;
    /** Map 구조의 끝을 알립니다. */
    virtual void EndMap() = 0;

    /** Map 엔트리의 Key 부분 시작을 알립니다. */
    virtual void BeginMapKey() = 0;
    /** Map 엔트리의 Key 부분 끝을 알립니다. */
    virtual void EndMapKey() = 0;

    /** Map 엔트리의 Value 부분 시작을 알립니다. */
    virtual void BeginMapValue() = 0;
    /** Map 엔트리의 Value 부분 끝을 알립니다. */
    virtual void EndMapValue() = 0;

    /**
     * 로우 레벨 바이트 데이터를 처리합니다.
     * 바이너리 포맷에서는 직접 memcpy, 텍스트 포맷에서는 Base64 등으로 변환할 수 있습니다.
     */
    virtual void SerializeBytes(void* data, uint64 size) = 0;

protected:
    /**
     * 다음에 직렬화될 값의 이름(Key)에 대한 힌트를 제공합니다.
     * @param name 변수의 이름
     */
    virtual void HintNextName(StringView name) = 0;

    // --- 스칼라(Primitive) 타입 처리 ---
    virtual void SerializeBool(bool& value) = 0;
    virtual void SerializeInt8(int8& value) = 0;
    virtual void SerializeUInt8(uint8& value) = 0;
    virtual void SerializeInt16(int16& value) = 0;
    virtual void SerializeUInt16(uint16& value) = 0;
    virtual void SerializeInt32(int32& value) = 0;
    virtual void SerializeUInt32(uint32& value) = 0;
    virtual void SerializeInt64(int64& value) = 0;
    virtual void SerializeUInt64(uint64& value) = 0;
    virtual void SerializeFloat(float& value) = 0;
    virtual void SerializeDouble(double& value) = 0;

    // --- 엔진 타입 처리 ---
    virtual void SerializeString(String& value) = 0;
    virtual void SerializeStringName(StringName& value) = 0;
    virtual void SerializeGuid(Guid& value) = 0;
    virtual void SerializeTypeId(TypeId& value) = 0;

protected:
    explicit Archive(EArchiveMode in_mode);

    EArchiveMode mode;
    std::unique_ptr<String> error_message;
};

namespace detail
{
/** Array-like 컨테이너 직렬화 (Array, FixedArray) */
template <traits::ArrayLike Container>
void SerializeArrayContainer(Archive& ar, Container& container);

/** Set-like 컨테이너 직렬화 (HashSet, Set, FlatSet) */
template <traits::SetLike Container>
void SerializeSetContainer(Archive& ar, Container& container);

/** Map-like 컨테이너 직렬화 (HashMap, Map, FlatMap) */
template <traits::MapLike Container>
void SerializeMapContainer(Archive& ar, Container& container);
}  // namespace detail


template <typename T>
Archive& Archive::operator<<(T& value)
{
    using PureType = std::remove_cvref_t<T>;

    // Bool (std::is_arithmetic_v<bool> == true 이므로 먼저 처리)
    if constexpr (std::same_as<PureType, bool>)
    {
        SerializeBool(value);
    }

    // 산술 타입 (POD)
    else if constexpr (std::is_arithmetic_v<PureType>)
    {
        if constexpr (std::same_as<PureType, int8>)        { SerializeInt8(value);   }
        else if constexpr (std::same_as<PureType, uint8>)  { SerializeUInt8(value);  }
        else if constexpr (std::same_as<PureType, int16>)  { SerializeInt16(value);  }
        else if constexpr (std::same_as<PureType, uint16>) { SerializeUInt16(value); }
        else if constexpr (std::same_as<PureType, int32>)  { SerializeInt32(value);  }
        else if constexpr (std::same_as<PureType, uint32>) { SerializeUInt32(value); }
        else if constexpr (std::same_as<PureType, int64>)  { SerializeInt64(value);  }
        else if constexpr (std::same_as<PureType, uint64>) { SerializeUInt64(value); }
        else if constexpr (std::same_as<PureType, float>)  { SerializeFloat(value);  }
        else if constexpr (std::same_as<PureType, double>) { SerializeDouble(value); }
        else
        {
            static_assert(traits::AlwaysFalse<T>, "Unsupported arithmetic type for serialization.");
        }
    }

    // Enum -> underlying type으로 변환 후 재귀
    else if constexpr (std::is_enum_v<PureType>)
    {
        using UnderlyingType = std::underlying_type_t<PureType>;
        UnderlyingType temp = static_cast<UnderlyingType>(value);
        *this << temp;
        if (IsLoading())
        {
            value = static_cast<T>(temp);
        }
    }

    // 엔진 특수 타입
    else if constexpr (std::same_as<PureType, String>)
    {
        SerializeString(value);
    }
    else if constexpr (std::same_as<PureType, StringName>)
    {
        SerializeStringName(value);
    }
    else if constexpr (std::same_as<PureType, Guid>)
    {
        SerializeGuid(value);
    }
    else if constexpr (std::same_as<PureType, TypeId>)
    {
        SerializeTypeId(value);
    }

    // Array-like
    else if constexpr (traits::ArrayLike<PureType>)
    {
        detail::SerializeArrayContainer(*this, value);
    }
    // Set-like
    else if constexpr (traits::SetLike<PureType>)
    {
        detail::SerializeSetContainer(*this, value);
    }
    // Map-like
    else if constexpr (traits::MapLike<PureType>)
    {
        detail::SerializeMapContainer(*this, value);
    }

    // 커스텀 직렬화 함수가 있는 UDT (ADL 또는 멤버)
    else if constexpr (traits::Serializable<PureType>)
    {
        BeginObject();
        Serialize(*this, value);
        EndObject();
    }

    // Fallback - 리플렉션 시스템에 등록된 타입은 AutoSerialize로 직렬화
    else if constexpr (Reflectable<PureType>)
    {
        BeginObject();
        AutoSerialize(*this, TypeId::Get<PureType>(), &value);
        EndObject();
    }

    // Fallback - static_assert
    else
    {
        static_assert(
            traits::AlwaysFalse<T>,
            "No serialization method found for this type. "
            "Define 'void Serialize(Archive&, T&)' or register the type with the reflection system."
        );
    }

    return *this;
}

template <typename T>
Archive& Archive::operator<<(const T& value)
{
    SE_ASSERT(IsSaving() && "Cannot deserialize (Load) into a const object!");
    return operator<<(const_cast<T&>(value));
}
} // namespace se


// 컨테이너 직렬화 구현
#include "SimpleEngine/Core/Serialization/ContainerSerialize.inl"
