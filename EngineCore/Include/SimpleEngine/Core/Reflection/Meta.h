#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Types/BitFlags.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se
{
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

/**
 * 멤버 변수(Property)의 추가 메타데이터 정보
 * @todo C++26 Custom Annotation 이용해서 구조체 채워넣기
 */
struct PropertyMetadata
{
    // 에디터에 표시될 이름
    StringView display_name;

    // 에디터에 표시될 ToopTip
    StringView tooltip;

    // Property 비트 플래그
    BitFlags<EPropertyFlags> flags;

    // 숫자 슬라이더 min/max
    float range_min = 0.0f;
    float range_max = 0.0f;
    bool has_range = false;

    // // 네트워크 리플리케이트
    // bool is_replicated = false;
};

/**
 * 멤버 변수(Property)의 리플렉션 정보
 */
struct PropertyInfo
{
    // Property 이름
    StringView name;

    // Property 크기
    usize size;

    // Property가 속하는 클래스/구조체로부터 떨어진 거리
    usize offset;

    // Property에 대한 컴파일타임 타입 식별자
    TypeId type_id;

    // Property의 추가 메타데이터 정보
    PropertyMetadata metadata;
};

/**
 * 클래스/구조체의 리플렉션 정보
 */
struct TypeInfo
{
    // 클래스/구조체 이름
    StringView name;

    // 클래스/구조체 총합 크기
    usize size;

    // 타입 메타데이터 flag
    BitFlags<ETypeFlags> flags;

    // 클래스/구조체의 멤버 변수(Property) 목록
    Array<PropertyInfo> properties; // TODO: C++26때 FixedArray로 바꿔야 할 수 있음

    // 컴파일타임 타입 식별자
    TypeId type_id;

    // TODO: 상속 정보 추가
};
} // namespace se

