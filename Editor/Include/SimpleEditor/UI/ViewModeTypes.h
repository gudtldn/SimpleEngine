#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::editor
{
/** 기즈모 좌표계 모드 */
enum class ECoordinateSpace : u8
{
    World,
    Local,
};

/** 뷰포트 뷰 모드 */
enum class EViewMode : u8
{
    // 원근 뷰
    Perspective,

    // 직교 뷰
    Top,
    Bottom,
    Front,
    Back,
    Right,
    Left,
};
} // namespace se::editor
