#pragma once

#include "SimpleEngine/Core/Container/Optional.h"

#include <variant>


namespace se
{
/**
 * 여러 람다를 하나의 오버로드 집합으로 묶습니다. std::visit과 함께 사용합니다.
 */
template <typename... Fns>
struct Overloaded : Fns...
{
    using Fns::operator()...;
};

/** variant가 현재 Alt를 담고 있으면 그 참조를, 아니면 NullOpt를 반환합니다. */
template <typename Alt, typename... Ts>
[[nodiscard]] constexpr Optional<const Alt&> VariantGet(const std::variant<Ts...>& variant)
{
    if (const Alt* alt = std::get_if<Alt>(&variant))
    {
        return *alt;
    }
    return NullOpt;
}
} // namespace se
