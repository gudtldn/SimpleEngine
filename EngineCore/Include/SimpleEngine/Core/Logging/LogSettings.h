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
    static void EnableColor(bool enable) { ColorEnabled = enable; }
    static void SetForceColor(bool force) { ForceColor = force; }
    [[nodiscard]] FORCE_INLINE static bool IsColorEnabled() { return ColorEnabled; }
    [[nodiscard]] FORCE_INLINE static bool IsColorForced() { return ForceColor; }

    [[nodiscard]] static bool DetectColorSupport();

private:
    inline static bool ColorEnabled = true;
    inline static bool ForceColor = false;
};
}
