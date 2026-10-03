#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Utility/Debug.h"

#include <charconv>
#include <cmath>
#include <iterator>
#include <utility>


namespace se::text_archive
{
/** Int/Enum 노드의 폭과 부호를 오류 메시지에 쓸 타입 이름으로 바꿉니다. */
[[nodiscard]] inline StringView IntTypeName(EIntWidth width, bool is_signed)
{
    switch (width)
    {
    case EIntWidth::Bits8: return is_signed ? "i8" : "u8";
    case EIntWidth::Bits16: return is_signed ? "i16" : "u16";
    case EIntWidth::Bits32: return is_signed ? "i32" : "u32";
    case EIntWidth::Bits64: return is_signed ? "i64" : "u64";
    }
    SE_UNREACHABLE();
}

/**
 * value가 width와 부호로 표현할 수 있는 범위 안인지 확인합니다.
 * u64는 0 이상의 i64만 받습니다. i64를 넘는 u64는 10진 문자열로 따로 읽습니다.
 */
[[nodiscard]] inline bool FitsInWidth(i64 value, EIntWidth width, bool is_signed)
{
    switch (width)
    {
    case EIntWidth::Bits8: return is_signed ? std::in_range<i8>(value) : std::in_range<u8>(value);
    case EIntWidth::Bits16: return is_signed ? std::in_range<i16>(value) : std::in_range<u16>(value);
    case EIntWidth::Bits32: return is_signed ? std::in_range<i32>(value) : std::in_range<u32>(value);
    case EIntWidth::Bits64: return is_signed || value >= 0;
    }
    SE_UNREACHABLE();
}

/** 문서 위치 parent 아래에 있는 key의 위치를 만듭니다. 예: ("window", "width")는 "window.width", ("", "vfs")는 "vfs" */
[[nodiscard]] inline String JoinPath(StringView parent, StringView key)
{
    if (parent.IsEmpty())
    {
        return { key };
    }
    return String::Format("{}.{}", parent, key);
}

/**
 * f32 값을 텍스트에 저장할 f64로 바꿉니다. f32로 되읽히는 가장 짧은 10진 표현을 씁니다(0.1f는 0.1).
 * 그 표현을 f64로 읽은 값이 f32로 돌아오지 않으면 확장된 값을 그대로 씁니다.
 */
[[nodiscard]] inline f64 ToShortestF64(f32 value)
{
    if (!std::isfinite(value))
    {
        return value;
    }

    char buffer[32];
    const char* const end = std::to_chars(std::begin(buffer), std::end(buffer), value).ptr;
    f64 shortest = 0.0;
    std::from_chars(buffer, end, shortest);

    // 짧은 표현이 두 f32의 정확한 중간값으로 읽히면 짝수 쪽 이웃으로 반올림됨 (유한 f32 전체 중 2개)
    if (static_cast<f32>(shortest) != value)
    {
        return value;
    }
    return shortest;
}
} // namespace se::text_archive
