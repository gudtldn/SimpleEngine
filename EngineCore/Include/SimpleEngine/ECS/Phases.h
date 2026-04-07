#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <concepts>


namespace se
{
// TODO: 추후 C++26때 Custom Annotation으로 대체
/** 시스템 실행 순서를 분류하는 기반 Phase 태그 */
struct Phase{};

/** 월드 생성 후 1회만 실행되는 Phase (리소스 로드, 카메라 설정 등) */
struct StartupPhase : Phase{};

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

/** Schedule Stage의 실행 모드 */
enum class EScheduleMode : uint8
{
    Once,          // 1회 실행 후 제거 (Startup 등)
    EveryFrame,    // 매 프레임 실행 (PreUpdate, Update, PostUpdate 등)
    FixedTimestep, // 고정 시간 간격으로 반복 실행 (FixedUpdate)
};
} // namespace se
