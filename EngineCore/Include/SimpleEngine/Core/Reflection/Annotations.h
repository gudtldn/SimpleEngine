#pragma once

#include "SimpleEngine/Core/Container/FixedString.h"

/** C++26으로 마이그레이션 시 CustomAnnotation을 바로 적용할 수 있도록 도와주는 헬퍼 매크로입니다. */
#define SE_ANNOTATION(...)


namespace se::meta
{
namespace target
{
struct Annotation {};

// Annotation 사용 위치 제한
struct Type   : Annotation {}; // 클래스, 구조체
struct Field  : Annotation {}; // 멤버 변수
struct Method : Annotation {}; // 멤버 함수

struct Member : Field, Method {};       // 멤버 변수 + 함수
struct Any    : Type, Field, Method {}; // 어디든 가능
} // namespace target

/** 클래스/구조체의 리플렉션 목적 정의 */
enum class EReflectUsage : uint8
{
    Default,       // (기본) 에디터 UI + 직렬화
    SerializeOnly, // UI 숨김 + 직렬화 (데이터 구조체, 패킷)
    EditorOnly,    // UI 표시 + 직렬화 제외 (런타임 디버그 윈도우 등)
    Internal       // UI 숨김 + 직렬화 제외 (코드 레벨 리플렉션만 필요)
};

namespace tags
{
// --- Type Tags ---
/** 리플렉션 진입점 태그 (Unreal의 UCLASS/USTRUCT 역할) */
struct Reflect : target::Type
{
    EReflectUsage usage;

    constexpr Reflect(EReflectUsage in_usage)
        : usage(in_usage) {}
};

/** ECS 컴포넌트 마킹 */
struct Component : target::Type {};

/** 추상 클래스 마킹 (인스턴스화 방지) */
struct Abstract  : target::Type {};

// --- Field Tags ---
/** 리플렉션 프로퍼티 진입점 태그 (Unreal의 UPROPERTY 역할) */
struct Property  : target::Field {};

// Access & Visibility
struct ReadOnly  : target::Field {}; // 에디터에서 읽기만 가능하게 설정 (Read Only)
struct Hidden    : target::Field {}; // 에디터 UI에 표시하지 않음 (Internal Data)

// Serialization
struct Transient : target::Field {}; // 직렬화/역직렬화 대상에서 제외

// UI Hints
struct Advanced  : target::Field {}; // 별도의 상세 탭(Advanced)에 표시


// --- Payload Tags ---
/**
 * 문자열 Payload를 가지는 태그의 기반 클래스입니다.
 * FixedString<N>은 N에 따라 다른 타입이 되므로, std::derived_from으로 통일 감지합니다.
 *
 * @note C++20에서 const char*(포인터)를 포함하는 타입은 NTTP로 사용할 수 없습니다.
 *       C++26 Custom Annotation에서도 동일한 제약이 적용됩니다.
 *       FixedString<N>은 char[N] 배열을 직접 저장하여 structural type 요구사항을 만족합니다.
 */
struct DisplayNameBase : target::Member {};
struct CategoryBase    : target::Member {};
struct TooltipBase     : target::Member {};

/** 에디터에서 표시할 이름입니다. */
template <FixedString Str>
struct DisplayName : DisplayNameBase
{
    /** static constexpr 이므로 StringView가 가리킬 수 있는 영구 저장소를 제공합니다. */
    static constexpr FixedString value = Str;
};

/** 에디터에서 카테고리를 나누는 용도로 사용합니다. */
template <FixedString Str>
struct Category : CategoryBase
{
    static constexpr FixedString value = Str;
};

/** 에디터에서 마우스를 올렸을 때 표시할 도움말입니다. */
template <FixedString Str>
struct Tooltip : TooltipBase
{
    static constexpr FixedString value = Str;
};

/** 숫자 데이터에 슬라이더 UI를 제공합니다. */
struct Range : target::Field
{
    float min;
    float max;

    constexpr Range(float in_min, float in_max)
        : min(in_min), max(in_max) {}
};

/** 숫자 데이터에 슬라이더 UI를 제공하고 실제 데이터의 논리적 범위를 제한합니다. */
struct Clamp : target::Field
{
    float min;
    float max;

    constexpr Clamp(float in_min, float in_max)
        : min(in_min), max(in_max) {}
};
} // namespace tags

// --- Type Annotation ---
constexpr tags::Reflect   Reflect       = EReflectUsage::Default;       // (기본) 에디터 UI + 직렬화
constexpr tags::Reflect   SerializeOnly = EReflectUsage::SerializeOnly; // UI 숨김 + 직렬화 (데이터 구조체, 패킷)
constexpr tags::Reflect   EditorOnly    = EReflectUsage::EditorOnly;    // UI 표시 + 직렬화 제외 (런타임 디버그 윈도우 등)
constexpr tags::Reflect   Internal      = EReflectUsage::Internal;      // UI 숨김 + 직렬화 제외 (코드 레벨 리플렉션만 필요)

constexpr tags::Component Component;  // ECS 엔티티 컴포넌트
constexpr tags::Abstract  Abstract;   // 추상 클래스

// --- Field Annotation ---
constexpr tags::Property  Property;  // 기본적으로 R/W + 직렬화 대상

constexpr tags::ReadOnly  ReadOnly;  // 값 표시는 하되 수정 불가
constexpr tags::Hidden    Hidden;    // 리플렉션에 등록은 하나, 아무런 표시를 하지 않음.
constexpr tags::Transient Transient; // 직렬화 대상에서 제외
constexpr tags::Advanced  Advanced;  // 별도의 상세 탭(Advanced)에 표시

// --- Payload Annotations ---
/**
 * 에디터에서 표시할 이름입니다.
 * @code
 * SE_REFLECT_PROPERTY(member, meta::Property, meta::DisplayName<"Title">{})
 * @endcode
 */
template <FixedString Str>
using DisplayName = tags::DisplayName<Str>;

/**
 * 에디터에서 카테고리를 나누는 용도로 사용합니다.
 * @code
 * SE_REFLECT_PROPERTY(member, meta::Property, meta::Category<"Physics">{})
 * @endcode
 */
template <FixedString Str>
using Category = tags::Category<Str>;

/**
 * 에디터에서 마우스를 올렸을 때 표시할 도움말입니다.
 * @code
 * SE_REFLECT_PROPERTY(member, meta::Property, meta::Tooltip<"Help text">{})
 * @endcode
 */
template <FixedString Str>
using Tooltip = tags::Tooltip<Str>;

/** 숫자 데이터에 슬라이더 UI를 제공하고 입력 범위를 제한합니다. */
using Range = tags::Range;

/** 숫자 데이터에 슬라이더 UI를 제공하고 실제 값의 입력 범위를 제한합니다. */
using Clamp = tags::Clamp;
} // namespace se::meta
