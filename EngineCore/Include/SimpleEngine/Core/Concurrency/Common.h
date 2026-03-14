#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Enum.h"

#include <new>


namespace se
{
/** 캐시 라인 크기 - False Sharing 방지용 정렬 단위 */
constexpr usize SE_CACHE_LINE = std::hardware_destructive_interference_size;

/** Job이 실행될 스레드를 지정하는 열거형 */
enum class EJobThread : uint8
{
    Main,   // 메인 스레드에서 실행
    Worker, // Compute Worker 스레드에서 실행
};

/** Job의 실행 우선순위를 지정하는 열거형 */
enum class EJobPriority : uint8
{
    Critical, // 프레임 데드라인 관련 작업 (렌더링, 오디오)
    Normal,   // 일반 작업 (ECS Tick, 물리)
    Low,      // 백그라운드 작업 (DDC Cook, 프리페치)
};

/** 우선순위 레벨 개수 */
constexpr usize NUM_JOB_PRIORITIES = EnumCount<EJobPriority>();
} // namespace se
