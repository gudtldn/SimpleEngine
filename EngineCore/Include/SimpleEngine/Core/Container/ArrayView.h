#pragma once
#include <algorithm>
#include <compare>
#include <concepts>
#include <initializer_list>
#include <iterator>
#include <type_traits>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
// Forward declaration
template <typename T, typename Allocator>
class Array;

template <typename T, usize N>
class FixedArray;

namespace core
{
template <typename T>
class DefaultAllocator;
} // namespace core


/** 동적 크기를 나타내는 상수 */
constexpr usize DynamicExtent = static_cast<usize>(-1);


namespace details
{
/**
 * ArrayView의 크기 저장소
 * Extent가 동적이면 런타임 크기를 저장하고, 정적이면 컴파일 타임 상수 사용
 */
template <usize InExtent>
struct ArrayViewStorage
{
    constexpr ArrayViewStorage() noexcept = default;
    constexpr explicit ArrayViewStorage([[maybe_unused]] usize count) noexcept {}
    [[nodiscard]] static constexpr usize Len() noexcept { return InExtent; }
};

template <>
struct ArrayViewStorage<DynamicExtent>
{
    usize data_len = 0;

    constexpr ArrayViewStorage() noexcept = default;
    constexpr explicit ArrayViewStorage(usize count) noexcept
        : data_len(count)
    {
    }
    [[nodiscard]] constexpr usize Len() const noexcept { return data_len; }
};
} // namespace details


/**
 * 연속 메모리 영역에 대한 비소유(non-owning) View
 * @tparam T 요소의 타입
 * @tparam InExtent 요소 개수 (DynamicExtent이면 런타임에 결정)
 */
template <typename T, usize InExtent = DynamicExtent>
class ArrayView : private details::ArrayViewStorage<InExtent>
{
    using StorageType = details::ArrayViewStorage<InExtent>;

public:
    using ValueType = T;
    using SizeType = usize;
    using DifferenceType = isize;
    using IteratorType = T*;
    using ConstIteratorType = const T*;
    using ReverseIteratorType = std::reverse_iterator<IteratorType>;
    using ConstReverseIteratorType = std::reverse_iterator<ConstIteratorType>;

    static constexpr SizeType Extent = InExtent;

public:
    /** 빈 View를 생성합니다. (동적 크기 또는 크기 0만 가능) */
    constexpr ArrayView() noexcept
        requires (InExtent == DynamicExtent || InExtent == 0)
    = default;

    /** 포인터와 크기로부터 View를 생성합니다. (동적 크기) */
    constexpr ArrayView(T* ptr, SizeType count) noexcept
        requires (InExtent == DynamicExtent)
        : StorageType(count)
        , data_ptr(ptr)
    {
    }

    /** 포인터로부터 정적 크기 View를 생성합니다. */
    constexpr explicit ArrayView(T* ptr) noexcept
        requires (InExtent != DynamicExtent)
        : data_ptr(ptr)
    {
    }

    /** 두 포인터(begin, end)로부터 View를 생성합니다. (동적 크기) */
    constexpr ArrayView(T* first, T* last) noexcept
        requires (InExtent == DynamicExtent)
        : StorageType(static_cast<SizeType>(last - first))
        , data_ptr(first)
    {
    }

    /** C 스타일 배열로부터 View를 생성합니다. */
    template <SizeType N>
        requires (InExtent == DynamicExtent || InExtent == N)
    constexpr ArrayView(T (&arr)[N]) noexcept
        : StorageType(N)
        , data_ptr(arr)
    {
    }

    /** std::initializer_list로부터 View를 생성합니다. (동적 크기, const만 가능) */
    constexpr ArrayView(std::initializer_list<std::remove_const_t<T>> init_list) noexcept
        requires (InExtent == DynamicExtent) && std::is_const_v<T>
        : StorageType(init_list.size())
        , data_ptr(init_list.begin())
    {
    }

    /** Array로부터 View를 생성합니다. (동적 크기) */
    template <typename Allocator>
    ArrayView(Array<std::remove_const_t<T>, Allocator>& arr) noexcept
        requires (InExtent == DynamicExtent)
        : StorageType(arr.Len())
        , data_ptr(arr.Data())
    {
    }

    /** const Array로부터 View를 생성합니다. (동적 크기) */
    template <typename Allocator>
    ArrayView(const Array<std::remove_const_t<T>, Allocator>& arr) noexcept
        requires (InExtent == DynamicExtent) && std::is_const_v<T>
        : StorageType(arr.Len())
        , data_ptr(arr.Data())
    {
    }

    /** FixedArray로부터 View를 생성합니다. */
    template <usize N>
        requires (InExtent == DynamicExtent || InExtent == N)
    constexpr ArrayView(FixedArray<std::remove_const_t<T>, N>& arr) noexcept
        : StorageType(N)
        , data_ptr(arr.Data())
    {
    }

    /** const FixedArray로부터 View를 생성합니다. */
    template <usize N>
        requires (InExtent == DynamicExtent || InExtent == N) && std::is_const_v<T>
    constexpr ArrayView(const FixedArray<std::remove_const_t<T>, N>& arr) noexcept
        : StorageType(N)
        , data_ptr(arr.Data())
    {
    }

    /** 동일 크기의 비const View에서 const View로의 암시적 변환 */
    template <typename U, usize OtherExtent>
        requires std::is_const_v<T>
              && std::same_as<std::remove_const_t<T>, U>
              && (InExtent == DynamicExtent || OtherExtent == DynamicExtent || InExtent == OtherExtent)
    constexpr ArrayView(ArrayView<U, OtherExtent> other) noexcept
        : StorageType(other.Len())
        , data_ptr(other.Data())
    {
    }

    /** 정적 크기 View에서 동적 크기 View로의 암시적 변환 */
    template <usize OtherExtent>
        requires (InExtent == DynamicExtent) && (OtherExtent != DynamicExtent)
    constexpr ArrayView(ArrayView<T, OtherExtent> other) noexcept
        : StorageType(OtherExtent)
        , data_ptr(other.Data())
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
        if (idx >= Len())
        {
            return std::nullopt;
        }
        return data_ptr[idx];
    }

    /** 첫 번째 요소를 반환합니다. */
    [[nodiscard]] constexpr Optional<T&> Front() const noexcept
    {
        if (Len() == 0)
        {
            return std::nullopt;
        }
        return data_ptr[0];
    }

    /** 첫 번째 요소를 반환합니다. */
    [[nodiscard]] constexpr T& FrontChecked() const noexcept
    {
        SE_CONSTEXPR_ASSERT(Len() > 0, "FrontChecked() called on empty ArrayView");
        return data_ptr[0];
    }

    /** 마지막 요소를 반환합니다. */
    [[nodiscard]] constexpr Optional<T&> Back() const noexcept
    {
        if (Len() == 0)
        {
            return std::nullopt;
        }
        return data_ptr[Len() - 1];
    }

    /** 마지막 요소를 반환합니다. */
    [[nodiscard]] constexpr T& BackChecked() const noexcept
    {
        SE_CONSTEXPR_ASSERT(Len() > 0, "BackChecked() called on empty ArrayView");
        return data_ptr[Len() - 1];
    }

    /** 데이터 포인터를 반환합니다. */
    [[nodiscard]] constexpr T* Data() const noexcept { return data_ptr; }

public:
    /** 요소 개수를 반환합니다. */
    [[nodiscard]] constexpr SizeType Len() const noexcept { return StorageType::Len(); }

    /** 요소 개수를 반환합니다. (STL 호환) */
    [[nodiscard]] constexpr SizeType size() const noexcept { return Len(); }

    /** 바이트 크기를 반환합니다. */
    [[nodiscard]] constexpr SizeType ByteSize() const noexcept { return Len() * sizeof(T); }

    /** View가 비어있는지 확인합니다. */
    [[nodiscard]] constexpr bool IsEmpty() const noexcept { return Len() == 0; }

public:
    [[nodiscard]] constexpr IteratorType begin() const noexcept { return data_ptr; }
    [[nodiscard]] constexpr IteratorType end() const noexcept { return data_ptr + Len(); }
    [[nodiscard]] constexpr ConstIteratorType cbegin() const noexcept { return data_ptr; }
    [[nodiscard]] constexpr ConstIteratorType cend() const noexcept { return data_ptr + Len(); }

    [[nodiscard]] constexpr ReverseIteratorType rbegin() const noexcept { return ReverseIteratorType(end()); }
    [[nodiscard]] constexpr ReverseIteratorType rend() const noexcept { return ReverseIteratorType(begin()); }
    [[nodiscard]] constexpr ConstReverseIteratorType crbegin() const noexcept { return ConstReverseIteratorType(cend()); }
    [[nodiscard]] constexpr ConstReverseIteratorType crend() const noexcept { return ConstReverseIteratorType(cbegin()); }

public:
    /** 처음 Count개의 요소를 포함하는 View를 반환합니다. (컴파일 타임) */
    template <SizeType Count>
        requires (InExtent == DynamicExtent || Count <= InExtent)
    [[nodiscard]] constexpr ArrayView<T, Count> First() const noexcept
    {
        SE_CONSTEXPR_ASSERT(Count <= Len(), "First<N>() count exceeds view size");
        return ArrayView<T, Count>(data_ptr);
    }

    /** 마지막 Count개의 요소를 포함하는 View를 반환합니다. (컴파일 타임) */
    template <SizeType Count>
        requires (InExtent == DynamicExtent || Count <= InExtent)
    [[nodiscard]] constexpr ArrayView<T, Count> Last() const noexcept
    {
        SE_CONSTEXPR_ASSERT(Count <= Len(), "Last<N>() count exceeds view size");
        return ArrayView<T, Count>(data_ptr + Len() - Count);
    }

    /** 부분 View를 반환합니다. (컴파일 타임) */
    template <SizeType Offset, SizeType Count = DynamicExtent>
        requires (InExtent == DynamicExtent || Offset <= InExtent)
              && (Count == DynamicExtent || InExtent == DynamicExtent || Offset + Count <= InExtent)
    [[nodiscard]] constexpr auto Subview() const noexcept
    {
        SE_CONSTEXPR_ASSERT(Offset <= Len(), "Subview<Offset, Count>() offset exceeds view size");

        if constexpr (Count != DynamicExtent)
        {
            SE_CONSTEXPR_ASSERT(Offset + Count <= Len(), "Subview<Offset, Count>() exceeds view size");
            return ArrayView<T, Count>(data_ptr + Offset);
        }
        else if constexpr (InExtent != DynamicExtent)
        {
            return ArrayView<T, InExtent - Offset>(data_ptr + Offset);
        }
        else
        {
            return ArrayView<T, DynamicExtent>(data_ptr + Offset, Len() - Offset);
        }
    }

public:
    /** 처음 count개의 요소를 포함하는 View를 반환합니다. (런타임) */
    [[nodiscard]] constexpr ArrayView<T> First(SizeType count) const noexcept
    {
        return ArrayView<T>(data_ptr, std::min(count, Len()));
    }

    /** 마지막 count개의 요소를 포함하는 View를 반환합니다. (런타임) */
    [[nodiscard]] constexpr ArrayView<T> Last(SizeType count) const noexcept
    {
        const SizeType actual_count = std::min(count, Len());
        return ArrayView<T>(data_ptr + Len() - actual_count, actual_count);
    }

    /** 부분 View를 반환합니다. (런타임) */
    [[nodiscard]] constexpr ArrayView<T> Subview(SizeType offset, SizeType count = DynamicExtent) const noexcept
    {
        const SizeType actual_offset = std::min(offset, Len());
        const SizeType actual_count = std::min(count, Len() - actual_offset);
        return ArrayView<T>(data_ptr + actual_offset, actual_count);
    }

public:
    template <usize OtherExtent>
    [[nodiscard]] constexpr bool operator==(ArrayView<T, OtherExtent> other) const noexcept
        requires std::equality_comparable<T>
    {
        if (Len() != other.Len())
        {
            return false;
        }
        for (SizeType i = 0; i < Len(); ++i)
        {
            if (!(data_ptr[i] == other[i]))
            {
                return false;
            }
        }
        return true;
    }

    template <usize OtherExtent>
    [[nodiscard]] constexpr std::strong_ordering operator<=>(ArrayView<T, OtherExtent> other) const noexcept
        requires std::three_way_comparable<T>
    {
        const SizeType min_len = std::min(Len(), other.Len());
        for (SizeType i = 0; i < min_len; ++i)
        {
            if (auto cmp = data_ptr[i] <=> other[i]; cmp != 0)
            {
                return cmp;
            }
        }
        return Len() <=> other.Len();
    }

private:
    T* data_ptr = nullptr;
};


template <typename T>
ArrayView(T*, usize) -> ArrayView<T>;

template <typename T>
ArrayView(T*, T*) -> ArrayView<T>;

template <typename T, usize N>
ArrayView(T (&)[N]) -> ArrayView<T, N>;

template <typename T>
ArrayView(std::initializer_list<T>) -> ArrayView<const T>;

template <typename T, usize N>
ArrayView(FixedArray<T, N>&) -> ArrayView<T, N>;

template <typename T, usize N>
ArrayView(const FixedArray<T, N>&) -> ArrayView<const T, N>;
} // namespace se
