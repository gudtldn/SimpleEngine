#pragma once

/** 클래스나 구조체에 리플렉션용 메타데이터를 부여하는 매크로입니다. */
#define SE_TYPE_ANNOTATION(...)
/** 멤버 변수(프로퍼티)에 리플렉션용 메타데이터를 부여하는 매크로입니다. */
#define SE_PROPERTY(...)

namespace se::meta
{
namespace details
{
// ECS Architecture Tags
struct ComponentTag {}; // ECS 엔티티 컴포넌트
struct ResourceTag {};  // 전역 리소스 (싱글톤 데이터)
struct EventTag {};     // 이벤트 메시지

// Editor UI Tags
struct EditTag {};      // 에디터에서 읽기/쓰기가 가능하게 설정 (+직렬화 기본값)
struct ReadOnlyTag {};  // 에디터에서 읽기만 가능하게 설정 (+직렬화 기본값)
struct ColorTag {};     // Vector3/4를 색상 피커로 표시

struct SerializeTag {}; // 직렬화/역직렬화 강제
struct TransientTag {}; // 직렬화/역직렬화 제외
}

constexpr details::ComponentTag Component; // ECS 엔티티 컴포넌트
constexpr details::ResourceTag Resource;   // 전역 리소스 (싱글톤 데이터)
constexpr details::EventTag Event;         // 이벤트 메시지

constexpr details::EditTag Edit;           // 값 표시 및 수정 가능
constexpr details::ReadOnlyTag ReadOnly;   // 값 표시는 하되 수정 불가
constexpr details::ColorTag Color;         // Vector3/4를 색상 피커로 표시

constexpr details::SerializeTag Serialize; // 직렬화 강제
constexpr details::TransientTag Transient; // 직렬화(저장) 제외

/**
 * 숫자 데이터에 슬라이더 UI를 제공하고 입력 범위를 제한합니다.
 */
struct Range
{
    float min;
    float max;

    constexpr Range(float in_min, float in_max)
        : min(in_min), max(in_max) {}
};

/**
 * 에디터에서 마우스를 올렸을 때 표시할 도움말입니다.
 */
struct Tooltip
{
    const char* message;

    explicit constexpr Tooltip(const char* msg)
        : message(msg) {}
};

/**
 * 에디터에서 표시할 이름입니다.
 */
struct DisplayName
{
    const char* name;

    explicit constexpr DisplayName(const char* name)
        : name(name) {}
};
}
