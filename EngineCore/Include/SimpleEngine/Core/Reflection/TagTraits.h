#pragma once

#include "SimpleEngine/Core/Reflection/Meta.h"
#include "SimpleEngine/Traits/TypeTraits.h"


namespace se::detail
{
/**
 * 이 태그가 등록 시 RegistrationTrait 특수화(Apply)를 반드시 가져야 하는지 여부
 */
template <typename Tag>
struct HookRequiredTrait
{
    static constexpr bool VALUE = false;
};

/**
 * 리플렉션 등록 태그에 대한 후처리 훅의 확장점
 * 코어는 이 Trait만 알고, 특정 태그(예: ECS Component)에 대한 실제 처리는
 * 그 태그를 소비하는 모듈(ECS 등)이 특수화합니다.
 * primary 템플릿은 비어 있으며, Apply<T>() 존재 여부로 "구현됐는지"를 판단합니다.
 *
 * @note Apply<T>는 반드시 제약이 없어야 합니다. 제약(requires)이 걸리면 조건에서 탈락한 T에 대해
 *       존재 감지가 false가 되어 "include 누락"으로 오진되거나 조용히 건너뛰게 됩니다.
 *       타입 요구조건은 Apply 본문 안의 static_assert로 검증하세요.
 */
template <typename Tag>
struct RegistrationTrait
{
};

/**
 * 태그가 ETypeFlags에 기여하는 값입니다. 태그 정의 시점(Annotations.h)에 특수화합니다.
 * 특수화가 없으면 static_assert로 실패하여, 분류되지 않은 태그를 컴파일 타임에 잡아냅니다.
 */
template <typename Tag>
struct TypeFlagTrait
{
    static_assert(se::traits::AlwaysFalse<Tag>, "TypeFlagTrait<Tag> specialization missing.");
};

/**
 * 태그가 PropertyMetadata에 기여하는 동작입니다. 태그 정의 시점(Annotations.h)에 특수화합니다.
 * Range처럼 태그 인스턴스의 값(min/max)이 필요한 경우를 위해 타입이 아닌 인스턴스를 받습니다.
 */
template <typename Tag>
struct PropertyMetadataTrait
{
    static_assert(se::traits::AlwaysFalse<Tag>, "PropertyMetadataTrait<Tag> specialization missing.");
};
} // namespace se::detail

// --- 반복되는 특수화 패턴을 줄이기 위한 헬퍼 매크로 ---
// 값 하나만 다르고 나머지 스캐폴딩이 완전히 동일한 3가지 경우에만 적용한다.
// 반드시 namespace se::detail { ... } 블록 안에서 호출해야 함. (기존 특수화와 동일한 위치 규약)
// payload 태그처럼 본문 로직 자체가 태그마다 다른 경우는 macro화하지 않음. (Annotations.h 참고)

/** HookRequiredTrait<Tag>::VALUE = true 특수화를 생성합니다. */
#define SE_HOOK_REQUIRED(Tag) \
    template <> struct HookRequiredTrait<Tag> { static constexpr bool VALUE = true; }

/** TypeFlagTrait<Tag>::VALUE = FlagValue 특수화를 생성합니다. */
#define SE_TYPE_FLAG(Tag, FlagValue) \
    template <> struct TypeFlagTrait<Tag> { static constexpr ETypeFlags VALUE = FlagValue; }

/** meta.flags |= FlagValue 하나만 수행하는 PropertyMetadataTrait<Tag> 특수화를 생성합니다. */
#define SE_PROPERTY_FLAG(Tag, FlagValue) \
    template <> struct PropertyMetadataTrait<Tag> \
    { \
        static constexpr void Apply(PropertyMetadata& meta, const Tag&) { meta.flags |= FlagValue; } \
    }
