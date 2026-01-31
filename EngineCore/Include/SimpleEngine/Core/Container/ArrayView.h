#pragma once
#include <algorithm>
#include <compare>
#include <initializer_list>
#include <iterator>
#include <type_traits>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
// Forward declaration
template <typename T, typename Allocator>
class Array;

namespace core
{
template <typename T>
class DefaultAllocator;
} // namespace core


/**
 * 연속 메모리 영역에 대한 비소유(non-owning) View
 * @tparam T 요소의 타입
 */
template <typename T>
class ArrayView
{
public:
    using ValueType = T;
    using SizeType = usize;
    using DifferenceType = isize;
    using IteratorType = T*;
    using ConstIteratorType = const T*;
    using ReverseIteratorType = std::reverse_iterator<IteratorType>;
    using ConstReverseIteratorType = std::reverse_iterator<ConstIteratorType>;

public:
    /** 빈 View를 생성합니다. */
    constexpr ArrayView() noexcept = default;

    /** 포인터와 크기로부터 View를 생성합니다. */
    constexpr ArrayView(T* ptr, SizeType count) noexcept
        : data_ptr(ptr)
        , data_len(count)
    {
    }

    /** 두 포인터(begin, end)로부터 View를 생성합니다. */
    constexpr ArrayView(T* first, T* last) noexcept
        : data_ptr(first)
        , data_len(static_cast<SizeType>(last - first))
    {
    }

    /** C 스타일 배열로부터 View를 생성합니다. */
    template <SizeType N>
    constexpr ArrayView(T (&arr)[N]) noexcept
        : data_ptr(arr)
        , data_len(N)
    {
    }

    /** std::initializer_list로부터 View를 생성합니다. (const만 가능) */
    constexpr ArrayView(std::initializer_list<std::remove_const_t<T>> init_list) noexcept
        requires std::is_const_v<T>
        : data_ptr(init_list.begin())
        , data_len(init_list.size())
    {
    }

    /** Array로부터 View를 생성합니다. */
    template <typename Allocator>
    ArrayView(Array<std::remove_const_t<T>, Allocator>& arr) noexcept
        requires (!std::is_const_v<T>)
        : data_ptr(arr.Data())
        , data_len(arr.Len())
    {
    }

    /** const Array로부터 View를 생성합니다. */
    template <typename Allocator>
    ArrayView(const Array<std::remove_const_t<T>, Allocator>& arr) noexcept
        requires std::is_const_v<T>
        : data_ptr(arr.Data())
        , data_len(arr.Len())
    {
    }

    /** 비const View에서 const View로의 암시적 변환 */
    template <typename U>
        requires std::is_const_v<T> && std::is_same_v<std::remove_const_t<T>, U>
    constexpr ArrayView(ArrayView<U> other) noexcept
        : data_ptr(other.Data())
        , data_len(other.Len())
    {
    }

    constexpr ArrayView(const ArrayView&) noexcept = default;
    constexpr ArrayView& operator=(const ArrayView&) noexcept = default;

public:
    /** 인덱스 위치의 요소를 반환합니다. */
    [[nodiscard]] constexpr T& operator[](SizeType idx) const noexcept
    {
        return data_ptr[idx];
    }

    /** 인덱스 위치의 요소를 반환합니다. */
    [[nodiscard]] constexpr Optional<T&> At(SizeType idx) const noexcept
    {
        if (idx >= data_len)
        {
            return std::nullopt;
        }
        return data_ptr[idx];
    }

    /** 첫 번째 요소를 반환합니다. */
    [[nodiscard]] constexpr Optional<T&> Front() const noexcept
    {
        if (data_len == 0)
        {
            return std::nullopt;
        }
        return data_ptr[0];
    }

    /** 첫 번째 요소를 반환합니다. (범위 검사 없음) */
    [[nodiscard]] constexpr T& FrontChecked() const noexcept
    {
        SE_ASSERT(data_len > 0, "FrontChecked() called on empty ArrayView");
        return data_ptr[0];
    }

    /** 마지막 요소를 반환합니다. */
    [[nodiscard]] constexpr Optional<T&> Back() const noexcept
    {
        if (data_len == 0)
        {
            return std::nullopt;
        }
        return data_ptr[data_len - 1];
    }

    /** 마지막 요소를 반환합니다. (범위 검사 없음) */
    [[nodiscard]] constexpr T& BackChecked() const noexcept
    {
        SE_ASSERT(data_len > 0, "BackChecked() called on empty ArrayView");
        return data_ptr[data_len - 1];
    }

    /** 데이터 포인터를 반환합니다. */
    [[nodiscard]] constexpr T* Data() const noexcept { return data_ptr; }

public:
    /** 요소 개수를 반환합니다. */
    [[nodiscard]] constexpr SizeType Len() const noexcept { return data_len; }

    /** 요소 개수를 반환합니다. (STL 호환) */
    [[nodiscard]] constexpr SizeType size() const noexcept { return data_len; }

    /** 바이트 크기를 반환합니다. */
    [[nodiscard]] constexpr SizeType ByteSize() const noexcept { return data_len * sizeof(T); }

    /** View가 비어있는지 확인합니다. */
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return data_len == 0; }

public:
    [[nodiscard]] constexpr IteratorType begin() const noexcept { return data_ptr; }
    [[nodiscard]] constexpr IteratorType end() const noexcept { return data_ptr + data_len; }
    [[nodiscard]] constexpr ConstIteratorType cbegin() const noexcept { return data_ptr; }
    [[nodiscard]] constexpr ConstIteratorType cend() const noexcept { return data_ptr + data_len; }

    [[nodiscard]] constexpr ReverseIteratorType rbegin() const noexcept { return ReverseIteratorType(end()); }
    [[nodiscard]] constexpr ReverseIteratorType rend() const noexcept { return ReverseIteratorType(begin()); }
    [[nodiscard]] constexpr ConstReverseIteratorType crbegin() const noexcept { return ConstReverseIteratorType(cend()); }
    [[nodiscard]] constexpr ConstReverseIteratorType crend() const noexcept { return ConstReverseIteratorType(cbegin()); }

public:
    /** 처음 count개의 요소를 포함하는 View를 반환합니다. */
    [[nodiscard]] constexpr ArrayView First(SizeType count) const noexcept
    {
        return ArrayView(data_ptr, std::min(count, data_len));
    }

    /** 마지막 count개의 요소를 포함하는 View를 반환합니다. */
    [[nodiscard]] constexpr ArrayView Last(SizeType count) const noexcept
    {
        const SizeType actual_count = std::min(count, data_len);
        return ArrayView(data_ptr + data_len - actual_count, actual_count);
    }

    /** 부분 View를 반환합니다. */
    [[nodiscard]] constexpr ArrayView Subview(SizeType offset, SizeType count = static_cast<SizeType>(-1)) const noexcept
    {
        const SizeType actual_offset = std::min(offset, data_len);
        const SizeType actual_count = std::min(count, data_len - actual_offset);
        return ArrayView(data_ptr + actual_offset, actual_count);
    }

public:
    [[nodiscard]] constexpr bool operator==(ArrayView other) const noexcept
        requires std::equality_comparable<T>
    {
        if (data_len != other.data_len)
        {
            return false;
        }
        for (SizeType i = 0; i < data_len; ++i)
        {
            if (!(data_ptr[i] == other.data_ptr[i]))
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(ArrayView other) const noexcept
        requires std::three_way_comparable<T>
    {
        const SizeType min_len = std::min(data_len, other.data_len);
        for (SizeType i = 0; i < min_len; ++i)
        {
            if (auto cmp = data_ptr[i] <=> other.data_ptr[i]; cmp != 0)
            {
                return cmp;
            }
        }
        return data_len <=> other.data_len;
    }

private:
    T* data_ptr = nullptr;
    SizeType data_len = 0;
};


template <typename T>
ArrayView(T*, usize) -> ArrayView<T>;

template <typename T>
ArrayView(T*, T*) -> ArrayView<T>;

template <typename T, usize N>
ArrayView(T (&)[N]) -> ArrayView<T>;

template <typename T>
ArrayView(std::initializer_list<T>) -> ArrayView<const T>;
} // namespace se
