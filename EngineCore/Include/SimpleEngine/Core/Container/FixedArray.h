// ReSharper disable CppMemberFunctionMayBeStatic
#pragma once
#include <algorithm>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se
{
/**
 * 컴파일 타임에 크기가 고정되는 스택 기반 배열 컨테이너
 * @tparam T 요소의 타입
 * @tparam N 배열의 크기
 */
template <typename T, usize N>
class FixedArray
{
public:
    using ValueType = T;

public:
    /**
     * 배열의 모든 요소를 특정 값으로 채웁니다.
     * @param init_val 배열을 채울 값
     */
    constexpr void Fill(const T& init_val);

    /** 배열의 길이를 반환합니다. */
    [[nodiscard]] constexpr usize Len() const;

    /** 배열의 용량을 반환합니다. */
    [[nodiscard]] constexpr usize Capacity() const;

    /** 배열이 비어있는지 (N == 0) 확인합니다. */
    [[nodiscard]] constexpr bool IsEmpty() const;

    /** 첫 번째 요소에 대한 Optional 참조를 반환합니다. (N=0일 경우 nullopt) */
    [[nodiscard]] constexpr Optional<T&> Front();
    [[nodiscard]] constexpr Optional<const T&> Front() const;

    /** 마지막 요소에 대한 Optional 참조를 반환합니다. (N=0일 경우 nullopt) */
    [[nodiscard]] constexpr Optional<T&> Back();
    [[nodiscard]] constexpr Optional<const T&> Back() const;

    /**
     * 경계 검사를 수행하며 특정 인덱스의 요소에 대한 Optional 참조를 반환합니다.
     * @param index 접근할 요소의 인덱스
     * @return 요소에 대한 Optional 참조. 인덱스가 범위를 벗어나면 nullopt입니다.
     */
    [[nodiscard]] constexpr Optional<T&> At(usize index);
    [[nodiscard]] constexpr Optional<const T&> At(usize index) const;

    /** 내부 데이터 버퍼에 대한 포인터를 반환합니다. */
    [[nodiscard]] constexpr T* Data();
    [[nodiscard]] constexpr const T* Data() const;

public:
    [[nodiscard]] constexpr bool operator==(const FixedArray& other) const;

    [[nodiscard]] constexpr T& operator[](usize idx) noexcept;
    [[nodiscard]] constexpr const T& operator[](usize idx) const noexcept;

    // Iterator
    [[nodiscard]] constexpr T* begin() noexcept;
    [[nodiscard]] constexpr T* end() noexcept;
    [[nodiscard]] constexpr const T* begin() const noexcept;
    [[nodiscard]] constexpr const T* end() const noexcept;

    [[nodiscard]] constexpr std::reverse_iterator<T*> rbegin() noexcept;
    [[nodiscard]] constexpr std::reverse_iterator<T*> rend() noexcept;
    [[nodiscard]] constexpr std::reverse_iterator<const T*> rbegin() const noexcept;
    [[nodiscard]] constexpr std::reverse_iterator<const T*> rend() const noexcept;

public:
    T data[N];
};

template <typename T>
class FixedArray<T, 0>
{
public:
    using ValueType = T;

public:
    constexpr void Fill(const T&) {}

    [[nodiscard]] constexpr usize Len() const { return 0; }
    [[nodiscard]] constexpr usize Capacity() const { return 0; }
    [[nodiscard]] constexpr bool IsEmpty() const { return true; }

    [[nodiscard]] constexpr Optional<T&> Front() { return NullOpt; }
    [[nodiscard]] constexpr Optional<const T&> Front() const { return NullOpt; }

    [[nodiscard]] constexpr Optional<T&> Back() { return NullOpt; }
    [[nodiscard]] constexpr Optional<const T&> Back() const { return NullOpt; }

    [[nodiscard]] constexpr Optional<T&> At(usize) { return NullOpt; }
    [[nodiscard]] constexpr Optional<const T&> At(usize) const { return NullOpt; }

    [[nodiscard]] constexpr T* Data() { return nullptr; }
    [[nodiscard]] constexpr const T* Data() const { return nullptr; }

public:
    [[nodiscard]] constexpr bool operator==(const FixedArray& other) const { return other.IsEmpty(); }

    [[nodiscard]] constexpr T& operator[](usize) noexcept = delete;
    [[nodiscard]] constexpr const T& operator[](usize) const noexcept = delete;

    // Iterator
    [[nodiscard]] constexpr T* begin() noexcept { return nullptr; }
    [[nodiscard]] constexpr T* end() noexcept { return nullptr; }
    [[nodiscard]] constexpr const T* begin() const noexcept { return nullptr; }
    [[nodiscard]] constexpr const T* end() const noexcept { return nullptr; }

    [[nodiscard]] constexpr std::reverse_iterator<T*> rbegin() noexcept { return nullptr; }
    [[nodiscard]] constexpr std::reverse_iterator<T*> rend() noexcept { return nullptr; }
    [[nodiscard]] constexpr std::reverse_iterator<const T*> rbegin() const noexcept { return nullptr; }
    [[nodiscard]] constexpr std::reverse_iterator<const T*> rend() const noexcept { return nullptr; }
};

template <typename T, typename... U>
FixedArray(T, U...) -> FixedArray<T, 1 + sizeof...(U)>;

template <typename T, usize N>
FixedArray(const T (&)[N]) -> FixedArray<T, N>;

template <typename T, usize N>
constexpr void FixedArray<T, N>::Fill(const T& init_val)
{
    std::fill(begin(), end(), init_val);
}

template <typename T, usize N>
constexpr usize FixedArray<T, N>::Len() const
{
    return N;
}

template <typename T, usize N>
constexpr usize FixedArray<T, N>::Capacity() const
{
    return N;
}

template <typename T, usize N>
constexpr bool FixedArray<T, N>::IsEmpty() const
{
    return N == 0;
}

template <typename T, usize N>
constexpr Optional<T&> FixedArray<T, N>::Front()
{
    if constexpr (N > 0)
    {
        return data[0];
    }
    else
    {
        return NullOpt;
    }
}

template <typename T, usize N>
constexpr Optional<const T&> FixedArray<T, N>::Front() const
{
    if constexpr (N > 0)
    {
        return data[0];
    }
    else
    {
        return NullOpt;
    }
}

template <typename T, usize N>
constexpr Optional<T&> FixedArray<T, N>::Back()
{
    if constexpr (N > 0)
    {
        return data[N - 1];
    }
    else
    {
        return NullOpt;
    }
}

template <typename T, usize N>
constexpr Optional<const T&> FixedArray<T, N>::Back() const
{
    if constexpr (N > 0)
    {
        return data[N - 1];
    }
    else
    {
        return NullOpt;
    }
}

template <typename T, usize N>
constexpr Optional<T&> FixedArray<T, N>::At(usize index)
{
    if (index >= N)
    {
        return NullOpt;
    }
    return data[index];
}

template <typename T, usize N>
constexpr Optional<const T&> FixedArray<T, N>::At(usize index) const
{
    if (index >= N)
    {
        return NullOpt;
    }
    return data[index];
}

template <typename T, usize N>
constexpr T* FixedArray<T, N>::Data()
{
    return data;
}

template <typename T, usize N>
constexpr const T* FixedArray<T, N>::Data() const
{
    return data;
}

template <typename T, usize N>
constexpr bool FixedArray<T, N>::operator==(const FixedArray& other) const
{
    return std::equal(begin(), end(), other.begin());
}

template <typename T, usize N>
constexpr T& FixedArray<T, N>::operator[](usize idx) noexcept
{
    SE_ASSERT(idx < N, "Index out of bounds");
    return data[idx];
}

template <typename T, usize N>
constexpr const T& FixedArray<T, N>::operator[](usize idx) const noexcept
{
    SE_ASSERT(idx < N, "Index out of bounds");
    return data[idx];
}

template <typename T, usize N>
constexpr T* FixedArray<T, N>::begin() noexcept
{
    return data;
}

template <typename T, usize N>
constexpr T* FixedArray<T, N>::end() noexcept
{
    return data + N;
}

template <typename T, usize N>
constexpr const T* FixedArray<T, N>::begin() const noexcept
{
    return data;
}

template <typename T, usize N>
constexpr const T* FixedArray<T, N>::end() const noexcept
{
    return data + N;
}

template <typename T, usize N>
constexpr std::reverse_iterator<T*> FixedArray<T, N>::rbegin() noexcept
{
    return std::reverse_iterator(end());
}

template <typename T, usize N>
constexpr std::reverse_iterator<T*> FixedArray<T, N>::rend() noexcept
{
    return std::reverse_iterator(begin());
}

template <typename T, usize N>
constexpr std::reverse_iterator<const T*> FixedArray<T, N>::rbegin() const noexcept
{
    return std::reverse_iterator(end());
}

template <typename T, usize N>
constexpr std::reverse_iterator<const T*> FixedArray<T, N>::rend() const noexcept
{
    return std::reverse_iterator(begin());
}

template <typename T, typename... Ts>
    requires (std::convertible_to<Ts, T> && ...)
constexpr auto MakeFixedArray(Ts&&... args)
{
    return FixedArray<T, sizeof...(Ts)>{ { static_cast<T>(std::forward<Ts>(args))... } };
}

template <typename... Ts>
    requires (sizeof...(Ts) > 0)
constexpr auto MakeFixedArray(Ts&&... args)
{
    using CommonType = std::common_type_t<Ts...>;
    return MakeFixedArray<CommonType>(std::forward<Ts>(args)...);
}
}  // namespace se
