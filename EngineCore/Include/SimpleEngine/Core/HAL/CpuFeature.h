#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * CPU의 하드웨어 기능 지원 여부를 제공하는 구조체
 */
struct SE_CORE_API CpuFeature
{
    // x86/x64 SIMD Support
    [[nodiscard]] static bool HasSSE();
    [[nodiscard]] static bool HasSSE2();
    [[nodiscard]] static bool HasSSE3();
    [[nodiscard]] static bool HasSSSE3();
    [[nodiscard]] static bool HasSSE4_1();
    [[nodiscard]] static bool HasSSE4_2();
    [[nodiscard]] static bool HasAVX();
    [[nodiscard]] static bool HasAVX2();
    [[nodiscard]] static bool HasFMA3();
    [[nodiscard]] static bool HasFMA4();
    [[nodiscard]] static bool HasAVX512F(); // AVX512 Foundation

    // ARM SIMD Support
    [[nodiscard]] static bool HasNEON();
};
} // namespace se
