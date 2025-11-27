#pragma once
#include <string_view>

#include "SimpleEngine/Core/Types/BitFlags.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::refl
{
/**
 * 프로퍼티 속성 비트 플래그
 */
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
    std::string_view display_name;

    // 에디터에 표시될 ToopTip
    std::string_view tooltip;

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
    std::string_view name;

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
    std::string_view name;

    // 클래스/구조체 총합 크기
    usize size;

    // 클래스/구조체의 멤버 변수(Property) 목록
    Array<PropertyInfo> properties; // TODO: C++26때 FixedArray로 바꿔야 할 수 있음

    // 컴파일타임 타입 식별자
    TypeId type_id;

    // TODO: 상속 정보 추가
};
}

