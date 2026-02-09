#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/StringView.h"
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

    // --- ECS Architecture ---
    Component   = 1 << 0, // 이 클래스는 ECS 컴포넌트입니다.
    Resource    = 1 << 1, // 이 클래스는 전역 리소스입니다.
    System      = 1 << 2, // 이 클래스는 로직 시스템입니다.
    Event       = 1 << 3, // 이 클래스는 이벤트 구조체입니다.

    // --- Object Nature ---
    Abstract    = 1 << 4, // 추상 클래스입니다 (인스턴스화 불가능).

    // --- Serialization ---
    Transient   = 1 << 5, // 이 타입 자체를 직렬화 대상에서 제외합니다.

    // --- Editor ---
    Hidden      = 1 << 6, // 에디터(ex: Add Component 메뉴)에서 숨깁니다.
};
SE_ENABLE_BITMASK_OPERATORS(ETypeFlags)

/** 프로퍼티 속성 비트 플래그 */
enum class EPropertyFlags : uint32
{
    None        = 0,
    Edit        = 1 << 0, // 에디터 수정 가능
    ReadOnly    = 1 << 1, // 에디터 읽기 전용
    Serialized  = 1 << 2, // 직렬화 대상
    Transient   = 1 << 3, // 직렬화 제외
    ColorPicker = 1 << 4, // 색상 피커 사용

    DefaultEdit = Edit | Serialized,
    DefaultReadOnly = ReadOnly | Serialized
};
SE_ENABLE_BITMASK_OPERATORS(EPropertyFlags)

/** 리플렉션 타입 분류 */
enum class ETypeKind : uint8
{
    Primitive, // 기본 자료형 (int, float, string 등)
    Struct,    // 구조체 및 클래스
    Container, // 배열, 맵 등의 컨테이너
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

    /** 에디터에 표시될 ToopTip */
    StringView tooltip;

    /** Property의 리플렉션 속성 */
    BitFlags<EPropertyFlags> flags{ EPropertyFlags::DefaultEdit };

    // Range (for Sliders)
    bool has_range = false;
    float range_min = 0.0f;
    float range_max = 0.0f;

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
    /** Property의 실제 메모리 주소를 반환합니다. */
    PtrFunc get_ptr;

    /** Property의 값을 가져옵니다.
     * @param instance: 해당 변수를 가진 객체의 주소
     * @param out_value: 복사된 값을 저장할 버퍼의 주소
     */
    GetterFunc getter;

    /** Property에 값을 설정합니다.
     * @param instance: 해당 변수를 가진 객체의 주소
     * @param in_value: 설정할 값이 들어있는 버퍼의 주소
     */
    SetterFunc setter;
};

/**
 * 멤버 변수(Property)의 리플렉션 정보
 */
struct PropertyInfo
{
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

public:
    template <typename T>
    [[nodiscard]] bool Is() const { return type_id == TypeId::Get<T>(); }
};

/**
 * 클래스/구조체의 리플렉션 정보
 */
struct TypeInfo
{
    using ConstructorFunc = void*(*)();
    using DestructorFunc  = void(*)(void*);
    using SerializeFunc   = void(*)(void* instance, Archive& ar);
    using DrawUIFunc      = void(*)(void* instance);

public:
    /** Type의 컴파일타임 타입 식별자 */
    TypeId type_id;

    union
    {
        // Kind == Struct 인 경우, 부모 클래스 ID
        TypeId base_type_id;

        // Kind == Container or Enum, InnerType or UnderlyingType의 ID
        TypeId inner_type_id;
    };

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

public:
    /** Instance를 생성하는 함수 (new T()) */
    ConstructorFunc constructor = nullptr;

    /** Instance를 소멸시키는 함수 (delete T) */
    DestructorFunc destructor = nullptr;

    /** 객체의 상태를 바이너리나 텍스트로 저장/불러오기 하는 함수 */
    SerializeFunc serialize = nullptr;

    /** 엔진 에디터나 디버그 도구에서 해당 객체를 UI로 렌더링하는 함수 */
    DrawUIFunc draw_ui = nullptr;

public:
    [[nodiscard]] TypeId GetBaseType() const { return kind == ETypeKind::Struct ? base_type_id : TypeId{}; }
    [[nodiscard]] TypeId GetInnerType() const { return (kind == ETypeKind::Container || kind == ETypeKind::Enum) ? inner_type_id : TypeId{}; }
};
} // namespace se
