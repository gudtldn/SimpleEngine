#pragma once

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <ranges>
#include <type_traits>


namespace se
{
/**
 * std::ranges::view를 감싸는 Rust 스타일 메서드 체인 래퍼
 * @tparam V 내부 view 타입 (std::ranges::view를 만족해야 합니다)
 *
 * 컨테이너의 Iter() 메서드로 생성하며, 두 가지 사용 방식을 지원합니다.
 * @code
 * // 방식 1: IterChain (Rust 친화)
 * arr.Iter().Filter(pred).Map(fn).Collect<Array<int>>();
 *
 * // 방식 2: ranges 직접 사용 (C++ 표준)
 * arr | std::views::filter(pred) | std::views::transform(fn) | std::ranges::to<std::vector>();
 * @endcode
 */
template <std::ranges::view V>
class IterChain
{
    template <std::ranges::view OtherV>
    friend class IterChain;

public:
    using ValueType = std::ranges::range_value_t<V>;

    constexpr explicit IterChain(V v) noexcept(std::is_nothrow_move_constructible_v<V>)
        : ranges_view(std::move(v))
    {
    }

public:
    // --- 지연 평가되는 함수 ---

    /**
     * 각 요소를 fn으로 변환합니다. (std::views::transform)
     * @param fn 변환 함수
     */
    template <typename Self, typename Fn>
        requires std::invocable<Fn, std::ranges::range_reference_t<V>>
    [[nodiscard]] auto Map(this Self&& self, Fn&& fn)
    {
        auto v = std::forward<Self>(self).ranges_view | std::views::transform(std::forward<Fn>(fn));
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /**
     * 조건을 만족하는 요소만 통과시킵니다. (std::views::filter)
     * @param pred 조건자
     */
    template <typename Self, typename Pred>
        requires std::predicate<Pred, std::ranges::range_reference_t<V>>
    [[nodiscard]] auto Filter(this Self&& self, Pred&& pred)
    {
        auto v = std::forward<Self>(self).ranges_view | std::views::filter(std::forward<Pred>(pred));
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /**
     * 각 요소를 range로 변환한 뒤 한 단계 평탄화합니다. (views::transform | views::join)
     * @param fn 요소를 range로 변환하는 함수
     */
    template <typename Self, typename Fn>
        requires std::invocable<Fn, std::ranges::range_reference_t<V>>
        && std::ranges::input_range<std::invoke_result_t<Fn, std::ranges::range_reference_t<V>>>
    [[nodiscard]] auto FlatMap(this Self&& self, Fn&& fn)
    {
        auto v = std::forward<Self>(self).ranges_view
            | std::views::transform(std::forward<Fn>(fn))
            | std::views::join;
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /** 앞에서 n개만 가져옵니다. (std::views::take) */
    template <typename Self>
    [[nodiscard]] auto Take(this Self&& self, usize n)
    {
        auto v = std::forward<Self>(self).ranges_view | std::views::take(static_cast<isize>(n));
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /** 앞에서 n개를 건너뜁니다. (std::views::drop) */
    template <typename Self>
    [[nodiscard]] auto Skip(this Self&& self, usize n)
    {
        auto v = std::forward<Self>(self).ranges_view | std::views::drop(static_cast<isize>(n));
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /**
     * 각 요소에 0-based 인덱스를 붙입니다. (std::views::enumerate)
     * @return std::tuple<index, value> 쌍의 IterChain
     */
    template <typename Self>
    [[nodiscard]] auto Enumerate(this Self&& self)
    {
        auto v = std::forward<Self>(self).ranges_view | std::views::enumerate;
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /**
     * 역방향으로 순회합니다. (std::views::reverse)
     * @note bidirectional_range에서만 사용 가능합니다.
     */
    template <typename Self>
        requires std::ranges::bidirectional_range<V>
    [[nodiscard]] auto Reverse(this Self&& self)
    {
        auto v = std::forward<Self>(self).ranges_view | std::views::reverse;
        return IterChain<decltype(v)>{ std::move(v) };
    }

    /**
     * 두 IterChain을 요소별로 묶습니다. (std::views::zip)
     * @return std::tuple<left, right> 쌍의 IterChain
     */
    template <typename Self, std::ranges::view OtherV>
    [[nodiscard]] auto Zip(this Self&& self, IterChain<OtherV> other)
    {
        auto v = std::views::zip(std::forward<Self>(self).ranges_view, std::move(other).ranges_view);
        return IterChain<decltype(v)>{ std::move(v) };
    }

public:
    // --- 즉시 실행되는 함수 ---

    /**
     * 모든 요소를 Container로 변환합니다.
     * @tparam Container 목표 컨테이너 타입. 항상 명시해야 합니다.
     * @code
     * arr.Iter().Filter(pred).Collect<Array<int>>();
     * arr.Iter().Collect<std::vector<int>>();
     * @endcode
     */
    template <typename Container, typename Self>
    [[nodiscard]] Container Collect(this Self&& self)
    {
        auto&& v = std::forward<Self>(self).ranges_view;
        if constexpr (requires { Container::FromRange(std::forward<decltype(v)>(v)); })
        {
            return Container::FromRange(std::forward<decltype(v)>(v));
        }
        else
        {
            return std::ranges::to<Container>(std::forward<decltype(v)>(v));
        }
    }

    /** 모든 요소에 fn을 적용합니다. */
    template <typename Self, typename Fn>
        requires std::invocable<Fn, std::ranges::range_reference_t<V>>
    void ForEach(this Self&& self, Fn&& fn)
    {
        std::ranges::for_each(std::forward<Self>(self).ranges_view, std::forward<Fn>(fn));
    }

    /**
     * 조건을 만족하는 첫 번째 요소의 값(복사본)을 반환합니다.
     * @return 조건을 만족하는 요소가 없으면 NullOpt를 반환합니다.
     */
    template <typename Self, typename Pred>
        requires std::predicate<Pred, std::ranges::range_reference_t<V>>
    [[nodiscard]] Optional<ValueType> Find(this Self&& self, Pred&& pred)
    {
        auto&& v = std::forward<Self>(self).ranges_view;
        const auto it = std::ranges::find_if(v, std::forward<Pred>(pred));
        if (it == std::ranges::end(v))
        {
            return NullOpt;
        }
        return *it;
    }

    /** 조건을 만족하는 요소가 하나라도 있으면 true를 반환합니다. */
    template <typename Self, typename Pred>
        requires std::predicate<Pred, std::ranges::range_reference_t<V>>
    [[nodiscard]] bool Any(this Self&& self, Pred&& pred)
    {
        return std::ranges::any_of(std::forward<Self>(self).ranges_view, std::forward<Pred>(pred));
    }

    /** 모든 요소가 조건을 만족하면 true를 반환합니다. */
    template <typename Self, typename Pred>
        requires std::predicate<Pred, std::ranges::range_reference_t<V>>
    [[nodiscard]] bool All(this Self&& self, Pred&& pred)
    {
        return std::ranges::all_of(std::forward<Self>(self).ranges_view, std::forward<Pred>(pred));
    }

    /** 요소의 수를 반환합니다. */
    template <typename Self>
    [[nodiscard]] usize Count(this Self&& self)
    {
        return static_cast<usize>(std::ranges::distance(std::forward<Self>(self).ranges_view));
    }

    /**
     * 초기값 init에서 시작해 fn을 왼쪽부터 누적 적용합니다.
     * @param init 초기 누산값
     * @param fn (accumulator, element) -> new_accumulator
     */
    template <typename Self, typename Init, typename Fn>
        requires std::invocable<Fn, Init, std::ranges::range_reference_t<V>>
    [[nodiscard]] Init Fold(this Self&& self, Init init, Fn&& fn)
    {
        return std::ranges::fold_left(std::forward<Self>(self).ranges_view, std::move(init), std::forward<Fn>(fn));
    }

    /** 모든 요소의 합을 반환합니다. (초기값 ValueType{}) */
    template <typename Self>
    [[nodiscard]] ValueType Sum(this Self&& self)
        requires traits::NumberType<ValueType>
    {
        return std::ranges::fold_left(std::forward<Self>(self).ranges_view, ValueType{}, std::plus<ValueType>{});
    }

    /** 모든 요소의 곱을 반환합니다. (초기값 ValueType{1}) */
    template <typename Self>
    [[nodiscard]] ValueType Product(this Self&& self)
        requires traits::NumberType<ValueType>
    {
        return std::ranges::fold_left(std::forward<Self>(self).ranges_view, ValueType{ 1 }, std::multiplies<ValueType>{});
    }

    /**
     * 최솟값을 반환합니다.
     * @return 비어있으면 NullOpt를 반환합니다.
     */
    template <typename Self>
    [[nodiscard]] Optional<ValueType> Min(this Self&& self)
        requires std::totally_ordered<ValueType>
    {
        auto&& v = std::forward<Self>(self).ranges_view;
        const auto it = std::ranges::min_element(v);
        if (it == std::ranges::end(v))
        {
            return NullOpt;
        }
        return *it;
    }

    /**
     * 최댓값을 반환합니다.
     * @return 비어있으면 NullOpt를 반환합니다.
     */
    template <typename Self>
    [[nodiscard]] Optional<ValueType> Max(this Self&& self)
        requires std::totally_ordered<ValueType>
    {
        auto&& v = std::forward<Self>(self).ranges_view;
        const auto it = std::ranges::max_element(v);
        if (it == std::ranges::end(v))
        {
            return NullOpt;
        }
        return *it;
    }

    [[nodiscard]] auto begin() { return std::ranges::begin(ranges_view); }
    [[nodiscard]] auto end() { return std::ranges::end(ranges_view); }
    [[nodiscard]] auto begin() const requires std::ranges::range<const V> { return std::ranges::begin(ranges_view); }
    [[nodiscard]] auto end() const requires std::ranges::range<const V> { return std::ranges::end(ranges_view); }

private:
    V ranges_view;
};

template <std::ranges::view V>
IterChain(V) -> IterChain<V>;
} // namespace se
