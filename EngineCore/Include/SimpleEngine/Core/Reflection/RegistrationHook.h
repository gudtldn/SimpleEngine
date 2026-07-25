#pragma once


namespace se::detail
{
/**
 * 이 태그가 등록 시 RegistrationHook 특수화(OnRegister)를 반드시 가져야 하는지 여부
 */
template <typename Tag>
struct HookRequired
{
    static constexpr bool VALUE = false;
};

/**
 * 리플렉션 등록 태그에 대한 후처리 훅의 확장점
 * 코어는 이 Trait만 알고, 특정 태그(예: ECS Component)에 대한 실제 처리는
 * 그 태그를 소비하는 모듈(ECS 등)이 특수화합니다.
 * primary 템플릿은 비어 있으며, OnRegister<T>() 존재 여부로 "구현됐는지"를 판단합니다.
 */
template <typename Tag>
struct RegistrationHook
{
};
} // namespace se::detail
