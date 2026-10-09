#pragma once

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

namespace se::display
{
/** 에디터에 표시하지 않는 대상에 붙이는 어노테이션입니다. */
struct HiddenAnnotation{};
inline constexpr HiddenAnnotation Hidden{};

/** 에디터에서 수정할 수 없는 필드에 붙이는 어노테이션입니다. */
struct ReadOnlyAnnotation{};
inline constexpr ReadOnlyAnnotation ReadOnly{};

/** 에디터에 필드 이름 대신 보일 이름입니다. */
struct DisplayNameAnnotation{ StringView value; };
[[nodiscard]] consteval DisplayNameAnnotation DisplayName(StringView value)
{
    return { .value = value };
}

/** 에디터에서 숫자 필드가 가질 수 있는 범위입니다. */
struct RangeAnnotation{ f32 min; f32 max; };
[[nodiscard]] consteval RangeAnnotation Range(f32 min, f32 max)
{
    return {
        .min = min,
        .max = max,
    };
}

/** 비트 플래그로 다루는 enum에 붙이는 어노테이션입니다. */
struct BitFlagsAnnotation{};
inline constexpr BitFlagsAnnotation BitFlags{};
} // namespace se::display
