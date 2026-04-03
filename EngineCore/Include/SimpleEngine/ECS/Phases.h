#pragma once

#include <concepts>


namespace se
{
// TODO: 추후 C++26때 Custom Annotation으로 대체
/** 시스템 실행 순서를 분류하는 기반 Phase 태그 */
struct Phase{};

/** Update 루프 이전에 실행되는 Phase */
struct PreUpdatePhase : Phase{};

/** 고정 시간 간격으로 실행되는 Phase (물리 등) */
struct FixedUpdatePhase : Phase{};

/** 메인 Update Phase */
struct UpdatePhase : Phase{};

/** Update 루프 이후에 실행되는 Phase */
struct PostUpdatePhase : Phase{};

// TODO: 추후 C++26때 Custom Annotation으로 대체
/** 시스템 Phase 타입 제약 조건 */
template <typename T>
concept PhaseType = std::derived_from<T, Phase>;
} // namespace se
