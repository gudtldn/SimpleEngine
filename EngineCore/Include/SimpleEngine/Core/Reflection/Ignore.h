#pragma once


namespace se
{
/** 리플렉션에서 서술하지 않는 필드에 붙이는 어노테이션입니다. */
struct IgnoreAnnotation{};
inline constexpr IgnoreAnnotation Ignore{};
} // namespace se
