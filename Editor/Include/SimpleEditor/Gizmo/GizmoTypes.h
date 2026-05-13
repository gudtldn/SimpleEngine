#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::editor
{
/** 기즈모 조작 모드 */
enum class EGizmoMode : u8
{
    Translate, // 이동
    Rotate,    // 회전
    Scale      // 스케일
};

/** 기즈모 축 또는 평면 식별자 (Picking/Highlight에서 사용) */
enum class EGizmoAxis : u8
{
    None = 0, // 선택 없음
    X,        // X축 (빨강)
    Y,        // Y축 (초록)
    Z,        // Z축 (파랑)
    XY,       // XY 평면
    XZ,       // XZ 평면
    YZ,       // YZ 평면
    All       // 전체 (균등 스케일 등)
};
} // namespace se::editor
