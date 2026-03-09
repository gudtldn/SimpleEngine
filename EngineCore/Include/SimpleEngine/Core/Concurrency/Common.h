#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
/** 캐시 라인 크기 - False Sharing 방지용 정렬 단위 */
constexpr usize SE_CACHE_LINE = 64;
} // namespace se
