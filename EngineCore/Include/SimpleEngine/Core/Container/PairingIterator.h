#pragma once

#include <compare>
#include <iterator>
#include <type_traits>
#include <utility>


namespace se
{
/**
 * 두 개의 병렬 이터레이터를 묶어 pair 프록시 참조를 반환하는 Random Access Iterator
 * SoA(Structure of Arrays) 기반 FlatMap에서 keys/values 배열을 동시에 순회합니다.
 *
 * @tparam KeyIter 키 배열의 이터레이터 타입
 * @tparam ValueIter 값 배열의 이터레이터 타입
 */
template <typename KeyIter, typename ValueIter>
class PairingIterator
{
public:
    using KeyReference = std::iter_reference_t<KeyIter>;
    using ValueReference = std::iter_reference_t<ValueIter>;

    using iterator_concept = std::random_access_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<
        std::remove_cvref_t<KeyReference>,
        std::remove_cvref_t<ValueReference>
    >;
    using reference = std::pair<KeyReference, ValueReference>;

private:
    /** operator->가 프록시 참조의 주소를 반환하기 위한 래퍼입니다. */
    class ArrowProxy
    {
    public:
        explicit ArrowProxy(reference ref) : ref(ref) {}
        const reference* operator->() const noexcept { return &ref; }

    private:
        reference ref;
    };

public:
    using pointer = ArrowProxy;

    PairingIterator() = default;

    constexpr PairingIterator(KeyIter key_it, ValueIter value_it) noexcept
        : key_iter(key_it)
        , value_iter(value_it)
    {
    }

    [[nodiscard]] reference operator*() const { return reference{*key_iter, *value_iter}; }
    [[nodiscard]] pointer operator->() const { return pointer{**this}; }

    [[nodiscard]] reference operator[](difference_type n) const
    {
        return *(*this + n);
    }

    PairingIterator& operator++()
    {
        ++key_iter;
        ++value_iter;
        return *this;
    }

    PairingIterator operator++(int)
    {
        auto old = *this;
        ++*this;
        return old;
    }

    PairingIterator& operator--()
    {
        --key_iter;
        --value_iter;
        return *this;
    }

    PairingIterator operator--(int)
    {
        auto old = *this;
        --*this;
        return old;
    }

    PairingIterator& operator+=(difference_type n)
    {
        key_iter += n;
        value_iter += n;
        return *this;
    }

    PairingIterator& operator-=(difference_type n)
    {
        key_iter -= n;
        value_iter -= n;
        return *this;
    }

    [[nodiscard]] PairingIterator operator+(difference_type n) const
    {
        auto tmp = *this;
        tmp += n;
        return tmp;
    }

    [[nodiscard]] PairingIterator operator-(difference_type n) const
    {
        auto tmp = *this;
        tmp -= n;
        return tmp;
    }

    [[nodiscard]] difference_type operator-(const PairingIterator& rhs) const
    {
        return key_iter - rhs.key_iter;
    }

    [[nodiscard]] friend PairingIterator operator+(difference_type n, const PairingIterator& it)
    {
        return it + n;
    }

    [[nodiscard]] bool operator==(const PairingIterator& rhs) const
    {
        return key_iter == rhs.key_iter;
    }

    [[nodiscard]] auto operator<=>(const PairingIterator& rhs) const
    {
        return key_iter <=> rhs.key_iter;
    }

    /** std::sort / std::ranges::sort가 사용하는 ADL swap */
    friend void iter_swap(const PairingIterator& a, const PairingIterator& b)
    {
        using std::iter_swap;
        iter_swap(a.key_iter, b.key_iter);
        iter_swap(a.value_iter, b.value_iter);
    }

    /** std::ranges 알고리즘이 사용하는 ADL iter_move */
    friend auto iter_move(const PairingIterator& it)
    {
        return std::pair<
            std::iter_rvalue_reference_t<KeyIter>,
            std::iter_rvalue_reference_t<ValueIter>
        >{
            std::ranges::iter_move(it.key_iter),
            std::ranges::iter_move(it.value_iter)
        };
    }

    [[nodiscard]] KeyIter KeyBase() const noexcept { return key_iter; }
    [[nodiscard]] ValueIter ValueBase() const noexcept { return value_iter; }

private:
    KeyIter key_iter{};
    ValueIter value_iter{};
};
} // namespace se
