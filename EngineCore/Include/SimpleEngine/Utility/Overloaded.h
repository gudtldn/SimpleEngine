#pragma once


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
} // namespace se
