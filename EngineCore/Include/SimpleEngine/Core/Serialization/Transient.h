#pragma once


namespace se
{
/** 저장하지 않는 타입이나 필드에 붙이는 어노테이션입니다. */
struct TransientAnnotation
{
};

inline constexpr TransientAnnotation Transient{};
} // namespace se
