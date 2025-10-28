#pragma once
#include <string_view>

#include "SimpleEngine/Reflection/TypeId.h"


namespace se::refl
{
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

    // // 숫자 슬라이더 min/max
    // float min_value = 0.0f;
    // float max_value = 1.0f;
    //
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

