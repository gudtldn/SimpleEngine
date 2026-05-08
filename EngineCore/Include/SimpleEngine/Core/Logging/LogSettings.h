#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/**
 * 색상 설정 관리 클래스
 */
class SE_CORE_API LogSettings
{
public:
    static void EnableColor(bool enable) { color_enabled = enable; }
    static void SetForceColor(bool force) { force_color = force; }
    [[nodiscard]] FORCE_INLINE static bool IsColorEnabled() { return color_enabled; }
    [[nodiscard]] FORCE_INLINE static bool IsColorForced() { return force_color; }

    [[nodiscard]] static bool DetectColorSupport();

private:
    inline static bool color_enabled = true;
    inline static bool force_color = false;
};
}
