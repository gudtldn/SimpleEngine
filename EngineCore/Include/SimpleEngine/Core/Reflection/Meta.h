#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Reflection/Enum.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/BitFlags.h"


namespace se
{
// forward declaration
class Archive;

/** 타입 속성 비트 플래그 */
enum class ETypeFlags : uint32
{
    None        = 0,

    // Identity
    IsComponent = 1 << 0, // ECS 컴포넌트 여부
    IsAbstract  = 1 << 1, // 추상 클래스 (인스턴스화 불가)

    // Serialization
    Transient   = 1 << 2, // 직렬화 대상에서 제외

    // Editor Visibility
    Hidden      = 1 << 3, // 에디터(ex: Add Component 메뉴)에서 숨기기

    // Enum-specific
    IsBitFlag   = 1 << 4, // 비트 플래그 조합 (체크박스 UI)
    IsUnsigned  = 1 << 5, // underlying type이 unsigned
};
SE_ENABLE_BITMASK_OPERATORS(ETypeFlags)

/** 프로퍼티 속성 비트 플래그 */
enum class EPropertyFlags : uint32
{
    None        = 0,

    // Access
    // 기본 Access 권한은 Edit
    ReadOnly    = 1 << 0, // 에디터에서 값 확인만 가능 (ReadOnly)
    Hidden      = 1 << 1, // 에디터 UI에 표시하지 않음 (Internal)

    // Serialization
    // 기본적으로는 직렬화 대상이며, Transient가 켜져있으면 제외합니다.
    Transient   = 1 << 2, // 직렬화(저장) 대상에서 제외

    // Networking
    // Replicated  = 1 << 3, // // 서버의 프로퍼티 변경사항을 클라이언트로 동기화(복제)
    // ReNotify 같은것도 있어야 함

    // UI Hints & Constraints
    Advanced    = 1 << 4, // 별도의 상세 탭(Advanced)에 표시
    HasRange    = 1 << 5, // range_min/max 값이 유효함 (UI Slider)
    HasClamp    = 1 << 6, // clamp_min/max 값이 유효함 (Logic Limit)
};
SE_ENABLE_BITMASK_OPERATORS(EPropertyFlags)

/** 리플렉션 타입 분류 */
enum class ETypeKind : uint8
{
    Primitive, // 기본 자료형 (int, float, string 등)
    Struct,    // 구조체 및 클래스
    Enum,      // 열거형
};

/**
 * 멤버 변수(Property)의 추가 메타데이터 정보
 * @todo C++26 Custom Annotation 이용해서 구조체 채워넣기
 */
struct PropertyMetadata
{
    /** 에디터에 표시될 이름 */
    StringView display_name;

    /** 카테고리 이름 */
    StringView category;

    /** 에디터에 표시될 Tooltip */
    StringView tooltip;

    /** Property의 리플렉션 속성 */
    BitFlags<EPropertyFlags> flags{ EPropertyFlags::None };

    // se::meta::Range (UI Slider)
    float range_min = 0.0f;
    float range_max = 0.0f;

    // se::meta::Clamp (Logic Hard Limit)
    double clamp_min = 0.0;
    double clamp_max = 0.0;

    // // 네트워크 리플리케이트
    // bool is_replicated = false;
};

/**
 * 멤버 변수(Property)의 접근자
 */
struct PropertyAccessor
{
    using PtrFunc    = void*(*)(void* instance);
    using GetterFunc = void(*)(const void* instance, void* out_value);
    using SetterFunc = void(*)(void* instance, const void* in_value);

public:
    /**
     * Property의 실제 메모리 주소를 반환합니다.
     * @param instance 해당 변수를 가진 객체의 주소
     */
    PtrFunc get_ptr = nullptr;

    /**
     * Property의 값을 가져옵니다.
     * @param instance 해당 변수를 가진 객체의 주소
     * @param out_value 복사된 값을 저장할 버퍼의 주소
     */
    GetterFunc getter = nullptr;

    /**
     * Property에 값을 설정합니다.
     * @param instance 해당 변수를 가진 객체의 주소
     * @param in_value 설정할 값이 들어있는 버퍼의 주소
     */
    SetterFunc setter = nullptr;
};

/**
 * 멤버 변수(Property)의 리플렉션 정보
 */
struct PropertyInfo
{
    using SerializeFunc = void(*)(Archive& ar, void* prop_ptr);

public:
    /** Property에 대한 컴파일타임 타입 식별자 */
    TypeId type_id;

    /** Property의 이름 */
    StringView name;

    /** Property 타입의 메모리 크기 (sizeof) */
    usize size;

    /** Property가 속하는 클래스/구조체로부터 떨어진 바이트 거리(Memory Offset) */
    usize offset;

public:
    /** Property에 부여된 부가적인 메타 정보 (예: 에디터 노출 여부, 범위 제한 등) */
    PropertyMetadata metadata;

    /** Property의 값을 읽거나 쓰기 위한 함수형 접근자 (Getter/Setter 인터페이스) */
    PropertyAccessor accessor;

    /** Property 단위 직렬화 콜백 (Archive::operator<< 디스패치를 통해 자동 생성됨) */
    SerializeFunc serialize = nullptr;

public:
    template <typename T>
    [[nodiscard]] bool Is() const { return type_id == TypeId::Get<T>(); }
};

/**
 * Interface의 메타데이터 정보
 */
struct InterfaceInfo
{
    /** Interface의 컴파일타임 타입 식별자 */
    TypeId type_id;

    /**
     * 객체 포인터를 해당 인터페이스 주소로 변환합니다. (Offset 조정)
     * @param instance 원본 객체의 포인터
     * @return 변환된 인터페이스 포인터 (다중 상속 대응)
     */
    void* (*caster)(void* instance) = nullptr;
};

/**
 * 클래스/구조체의 리플렉션 정보
 */
struct TypeInfo
{
    using ConstructorFunc = void*(*)();
    using DestructorFunc  = void(*)(void*);
    using SerializeFunc   = void(*)(Archive& ar, void* instance);
    using EnumEntriesFunc = void(*)(const EnumEntry*& out_data, usize& out_count);

public:
    /** Type의 컴파일타임 타입 식별자 */
    TypeId type_id;

    /**
     * 문맥에 따라 다른 역할의 타입 식별자
     * - Struct: 부모 클래스의 TypeId
     * - Array/Set: Element의 TypeId
     * - Map: Key의 TypeId
     * - Enum: Underlying Type의 TypeId
     */
    TypeId base_or_inner_id;

    /**
     * Map 전용: Value 타입의 TypeId
     * Map이 아닌 경우에는 사용되지 않습니다 (기본값: 빈 TypeId).
     */
    TypeId secondary_type_id;

    /** Type의 이름 */
    StringView name;

    /** Type의 전체 메모리 크기 (sizeof) */
    usize size;

    /** Type의 메모리 정렬 요구사항 (alignof) */
    usize alignment;

    /** 타입의 종류 (Primitive, Struct, Enum, Container등) */
    ETypeKind kind;

    /** 타입의 특성 Flag */
    BitFlags<ETypeFlags> flags;

    /** 해당 타입이 포함하는 멤버 변수(Property)들의 목록 */
    Array<PropertyInfo> properties;

    /** 해당 타입이 구현(상속)하는 인터페이스 목록 */
    HashMap<TypeId, InterfaceInfo> interfaces;

public:
    /** Instance를 생성하는 함수 (new T()) */
    ConstructorFunc constructor = nullptr;

    /** Instance를 소멸시키는 함수 (delete T) */
    DestructorFunc destructor = nullptr;

    /** 객체의 상태를 바이너리나 텍스트로 저장/불러오기 하는 함수 */
    SerializeFunc serialize = nullptr;

    /**
     * 타입이 소거된 Enum 항목 목록에 접근하는 함수
     * @note Struct/Primitive 타입에서는 nullptr
     */
    EnumEntriesFunc enum_entries = nullptr;
};
} // namespace se
