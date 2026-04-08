#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Math/MathSimd.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Core/Math/Vector4.h"
#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <concepts>
#include <mdspan>
#include <ranges>


namespace se::math
{
/**
 * row-major matrix template
 */
template <traits::FloatingType T>
struct alignas(sizeof(T) * 4) Matrix4x4Impl
{
public:
    using RealType = T;
    using SizeType = usize;
    using ExtentType = std::extents<SizeType, 4, 4>;

public:
    constexpr Matrix4x4Impl() = default;

    template <typename... Ts>
        requires ((std::convertible_to<Ts, T> && ...) && sizeof...(Ts) == 16)
    constexpr Matrix4x4Impl(Ts... values);

    constexpr Matrix4x4Impl(ArrayView<const T, 16> src);
    Matrix4x4Impl(ArrayView<const T> src);

    constexpr Matrix4x4Impl(const Vector4Impl<T>& r0, const Vector4Impl<T>& r1, const Vector4Impl<T>& r2, const Vector4Impl<T>& r3);

public:
    [[nodiscard]] static constexpr Matrix4x4Impl Identity();
    [[nodiscard]] static constexpr Matrix4x4Impl Zero();

public:
    [[nodiscard]] constexpr Matrix4x4Impl Transpose() const;
    [[nodiscard]] constexpr Matrix4x4Impl Inverse() const;

public:
    [[nodiscard]] constexpr Matrix4x4Impl operator+(const Matrix4x4Impl& rhs) const;
    constexpr Matrix4x4Impl& operator+=(const Matrix4x4Impl& rhs);

    [[nodiscard]] constexpr Matrix4x4Impl operator*(const Matrix4x4Impl& rhs) const;
    [[nodiscard]] constexpr Matrix4x4Impl operator*(T scalar) const;
    constexpr void operator*=(const Matrix4x4Impl& rhs);
    constexpr void operator*=(T scalar);

    [[nodiscard]] constexpr T& operator[](SizeType row, SizeType col) noexcept;
    [[nodiscard]] constexpr T operator[](SizeType row, SizeType col) const noexcept;

    [[nodiscard]] friend constexpr Vector4Impl<T> operator*(const Vector4Impl<T>& lhs, const Matrix4x4Impl& rhs)
    {
        Vector4Impl<T> result{};
        for (SizeType i = 0; i < 4; ++i)
        {
            result[i] =
                rhs[0, i] * lhs[0]
                + rhs[1, i] * lhs[1]
                + rhs[2, i] * lhs[2]
                + rhs[3, i] * lhs[3];
        }
        return result;
    }

public:
    FixedArray<T, 16> data;
};

template <traits::FloatingType T>
template <typename... Ts>
    requires ((std::convertible_to<Ts, T> && ...) && sizeof...(Ts) == 16)
constexpr Matrix4x4Impl<T>::Matrix4x4Impl(Ts... values)
    : data{ static_cast<T>(values)... }
{
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T>::Matrix4x4Impl(ArrayView<const T, 16> src)
{
    std::ranges::copy(src, data.begin());
}

template <traits::FloatingType T>
Matrix4x4Impl<T>::Matrix4x4Impl(ArrayView<const T> src)
{
    SE_ASSERT(src.Len() == 16, "Invalid ArrayView size.");
    std::ranges::copy(src, data.begin());
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T>::Matrix4x4Impl(const Vector4Impl<T>& r0, const Vector4Impl<T>& r1, const Vector4Impl<T>& r2, const Vector4Impl<T>& r3)
    : data{
        r0.x, r0.y, r0.z, r0.w,
        r1.x, r1.y, r1.z, r1.w,
        r2.x, r2.y, r2.z, r2.w,
        r3.x, r3.y, r3.z, r3.w
    }
{
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::Identity()
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::Zero()
{
    return {
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::Transpose() const
{
    Matrix4x4Impl result{};
    for (SizeType i = 0; i < 4; ++i)
    {
        result[i, 0] = (*this)[0, i];
        result[i, 1] = (*this)[1, i];
        result[i, 2] = (*this)[2, i];
        result[i, 3] = (*this)[3, i];
    }
    return result;
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::Inverse() const
{
    Matrix4x4Impl result;
    Matrix4x4Impl temp;
    T det[4];

    const Matrix4x4Impl& self = *this;
    temp[0, 0] = self[2, 2] * self[3, 3] - self[2, 3] * self[3, 2];
    temp[0, 1] = self[1, 2] * self[3, 3] - self[1, 3] * self[3, 2];
    temp[0, 2] = self[1, 2] * self[2, 3] - self[1, 3] * self[2, 2];

    temp[1, 0] = self[2, 2] * self[3, 3] - self[2, 3] * self[3, 2];
    temp[1, 1] = self[0, 2] * self[3, 3] - self[0, 3] * self[3, 2];
    temp[1, 2] = self[0, 2] * self[2, 3] - self[0, 3] * self[2, 2];

    temp[2, 0] = self[1, 2] * self[3, 3] - self[1, 3] * self[3, 2];
    temp[2, 1] = self[0, 2] * self[3, 3] - self[0, 3] * self[3, 2];
    temp[2, 2] = self[0, 2] * self[1, 3] - self[0, 3] * self[1, 2];

    temp[3, 0] = self[1, 2] * self[2, 3] - self[1, 3] * self[2, 2];
    temp[3, 1] = self[0, 2] * self[2, 3] - self[0, 3] * self[2, 2];
    temp[3, 2] = self[0, 2] * self[1, 3] - self[0, 3] * self[1, 2];

    det[0] = self[1, 1] * temp[0, 0] - self[2, 1] * temp[0, 1] + self[3, 1] * temp[0, 2];
    det[1] = self[0, 1] * temp[1, 0] - self[2, 1] * temp[1, 1] + self[3, 1] * temp[1, 2];
    det[2] = self[0, 1] * temp[2, 0] - self[1, 1] * temp[2, 1] + self[3, 1] * temp[2, 2];
    det[3] = self[0, 1] * temp[3, 0] - self[1, 1] * temp[3, 1] + self[2, 1] * temp[3, 2];

    const T determinant = self[0, 0] * det[0] - self[1, 0] * det[1] + self[2, 0] * det[2] - self[3, 0] * det[3];
    if (!IsFinite(determinant))
    {
        return Identity();
    }

    const T right_det = 1.0f / determinant;
    result[0, 0] = right_det * det[0];
    result[0, 1] = -right_det * det[1];
    result[0, 2] = right_det * det[2];
    result[0, 3] = -right_det * det[3];
    result[1, 0] = -right_det * (self[1, 0] * temp[0, 0] - self[2, 0] * temp[0, 1] + self[3, 0] * temp[0, 2]);
    result[1, 1] = right_det * (self[0, 0] * temp[1, 0] - self[2, 0] * temp[1, 1] + self[3, 0] * temp[1, 2]);
    result[1, 2] = -right_det * (self[0, 0] * temp[2, 0] - self[1, 0] * temp[2, 1] + self[3, 0] * temp[2, 2]);
    result[1, 3] = right_det * (self[0, 0] * temp[3, 0] - self[1, 0] * temp[3, 1] + self[2, 0] * temp[3, 2]);
    result[2, 0] = right_det * (
        self[1, 0] * (self[2, 1] * self[3, 3] - self[2, 3] * self[3, 1]) -
        self[2, 0] * (self[1, 1] * self[3, 3] - self[1, 3] * self[3, 1]) +
        self[3, 0] * (self[1, 1] * self[2, 3] - self[1, 3] * self[2, 1])
    );
    result[2, 1] = -right_det * (
        self[0, 0] * (self[2, 1] * self[3, 3] - self[2, 3] * self[3, 1]) -
        self[2, 0] * (self[0, 1] * self[3, 3] - self[0, 3] * self[3, 1]) +
        self[3, 0] * (self[0, 1] * self[2, 3] - self[0, 3] * self[2, 1])
    );
    result[2, 2] = right_det * (
        self[0, 0] * (self[1, 1] * self[3, 3] - self[1, 3] * self[3, 1]) -
        self[1, 0] * (self[0, 1] * self[3, 3] - self[0, 3] * self[3, 1]) +
        self[3, 0] * (self[0, 1] * self[1, 3] - self[0, 3] * self[1, 1])
    );
    result[2, 3] = -right_det * (
        self[0, 0] * (self[1, 1] * self[2, 3] - self[1, 3] * self[2, 1]) -
        self[1, 0] * (self[0, 1] * self[2, 3] - self[0, 3] * self[2, 1]) +
        self[2, 0] * (self[0, 1] * self[1, 3] - self[0, 3] * self[1, 1])
    );
    result[3, 0] = -right_det * (
        self[1, 0] * (self[2, 1] * self[3, 2] - self[2, 2] * self[3, 1]) -
        self[2, 0] * (self[1, 1] * self[3, 2] - self[1, 2] * self[3, 1]) +
        self[3, 0] * (self[1, 1] * self[2, 2] - self[1, 2] * self[2, 1])
    );
    result[3, 1] = right_det * (
        self[0, 0] * (self[2, 1] * self[3, 2] - self[2, 2] * self[3, 1]) -
        self[2, 0] * (self[0, 1] * self[3, 2] - self[0, 2] * self[3, 1]) +
        self[3, 0] * (self[0, 1] * self[2, 2] - self[0, 2] * self[2, 1])
    );
    result[3, 2] = -right_det * (
        self[0, 0] * (self[1, 1] * self[3, 2] - self[1, 2] * self[3, 1]) -
        self[1, 0] * (self[0, 1] * self[3, 2] - self[0, 2] * self[3, 1]) +
        self[3, 0] * (self[0, 1] * self[1, 2] - self[0, 2] * self[1, 1])
    );
    result[3, 3] = right_det * (
        self[0, 0] * (self[1, 1] * self[2, 2] - self[1, 2] * self[2, 1]) -
        self[1, 0] * (self[0, 1] * self[2, 2] - self[0, 2] * self[2, 1]) +
        self[2, 0] * (self[0, 1] * self[1, 2] - self[0, 2] * self[1, 1])
    );

    return result;
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::operator+(const Matrix4x4Impl& rhs) const
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 16; ++i)
    {
        ret.data[i] = data[i] + rhs.data[i];
    }
    return ret;
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T>& Matrix4x4Impl<T>::operator+=(const Matrix4x4Impl& rhs)
{
    for (SizeType i = 0; i < 16; ++i)
    {
        data[i] += rhs.data[i];
    }
    return *this;
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::operator*(const Matrix4x4Impl& rhs) const
{
    if consteval
    {
        Matrix4x4Impl ret{};
        for (SizeType i = 0; i < 4; ++i)
        {
            for (SizeType j = 0; j < 4; ++j)
            {
                ret[i, j] += (*this)[i, 0] * rhs[0, j];
                ret[i, j] += (*this)[i, 1] * rhs[1, j];
                ret[i, j] += (*this)[i, 2] * rhs[2, j];
                ret[i, j] += (*this)[i, 3] * rhs[3, j];
            }
        }
        return ret;
    }
    return simd::Matrix4x4Multiply(*this, rhs);
}

template <traits::FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::operator*(T scalar) const
{
    Matrix4x4Impl ret{};
    for (auto [n, ret_data] : ret.data | std::views::enumerate)
    {
        ret_data = data[n] * scalar;
    }
    return ret;
}

template <traits::FloatingType T>
constexpr void Matrix4x4Impl<T>::operator*=(const Matrix4x4Impl& rhs)
{
    *this = *this * rhs;
}

template <traits::FloatingType T>
constexpr void Matrix4x4Impl<T>::operator*=(T scalar)
{
    *this = *this * scalar;
}

template <traits::FloatingType T>
constexpr T& Matrix4x4Impl<T>::operator[](SizeType row, SizeType col) noexcept
{
    return data[(row * 4) + col];
}

template <traits::FloatingType T>
constexpr T Matrix4x4Impl<T>::operator[](SizeType row, SizeType col) const noexcept
{
    return data[(row * 4) + col];
}
} // namespace se::math
