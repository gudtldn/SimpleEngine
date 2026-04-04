#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/BitFlags.h"


namespace se::graphics
{
/**
 * 뷰포트의 렌더링 파이프라인 모드
 */
enum class ERenderingMode : uint8
{
    // 기본 렌더링 모드
    Lit,         // 기본 조명 렌더링
    Unlit,       // 조명 없이 베이스 컬러만 출력
    Wireframe,   // 와이어프레임

    // 디버그 렌더링 모드
    Normal,      // 오브젝트 공간 노멀 시각화
    WorldNormal, // 월드 공간 노멀 시각화
};

/**
 * 뷰포트별 렌더링 요소의 가시성을 제어하는 비트 플래그
 */
enum class EShowFlag : uint64
{
    None = 0,

    Grid = 1 << 0, // 에디터 그리드
    AABB = 1 << 1, // AABB 충돌 영역 표시

    All = 0xFFFF'FFFF,
};

SE_ENABLE_BITMASK_OPERATORS(EShowFlag)

using ShowFlags = BitFlags<EShowFlag>;
} // namespace se::graphics
