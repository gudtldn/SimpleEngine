#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::graphics
{
/** 사전 정의된 기본 GPU 샘플러 타입 */
enum class ESamplerType : uint8
{
    LinearRepeat, // 선형 + 반복 (일반 텍스처)
    LinearClamp,  // 선형 + 클램프 (UI/스프라이트)
    PointRepeat,  // 포인트 + 반복 (픽셀 아트)
    PointClamp,   // 포인트 + 클램프 (LUT 등 정밀 샘플링)
    Max,          // 전체 샘플러 개수
};
} // namespace se::graphics
