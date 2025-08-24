export module SimpleEngine.Math:Matrix;
import :MathUtility;
import :MathLiterals;
import :RotationTypes;
import :Vector3;
import :Vector4;

import SimpleEngine.Traits;
import SimpleEngine.Types;
import std;

import <cassert>;

using namespace se::traits::type_traits;


namespace se::math
{
/**
 * row-major matrix template
 */
template <FloatingType T>
struct alignas(16) Matrix4x4Impl
{
private:
    std::array<T, 16> data;

public:
    using RealType = T;
    using SizeType = size_t;
    using ExtentType = std::extents<SizeType, 4, 4>;

public:
    constexpr Matrix4x4Impl() = default;
    constexpr Matrix4x4Impl(std::span<T, 16> src);
    constexpr Matrix4x4Impl(std::span<T> src);
    template <typename... Ts>
        requires ((std::is_convertible_v<Ts, T> && ...) && sizeof...(Ts) == 16)
    constexpr Matrix4x4Impl(Ts... values);

    [[nodiscard]] static constexpr Matrix4x4Impl Identity();
    [[nodiscard]] static constexpr Matrix4x4Impl Zero();

public:
    [[nodiscard]] constexpr Matrix4x4Impl Transpose() const;
    [[nodiscard]] constexpr Matrix4x4Impl Inverse() const;

public:
    [[nodiscard]] T* GetData() noexcept;
    [[nodiscard]] const T* GetData() const noexcept;

    [[nodiscard]] auto GetView() noexcept;
    [[nodiscard]] auto GetView() const noexcept;

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
                rhs[i, 0] * lhs[0]
                + rhs[i, 1] * lhs[1]
                + rhs[i, 2] * lhs[2]
                + rhs[i, 3] * lhs[3];
        }
        return result;
    }
};

template <FloatingType T>
constexpr Matrix4x4Impl<T>::Matrix4x4Impl(std::span<T, 16> src)
{
    std::copy(src.begin(), src.end(), data.begin());
}

template <FloatingType T>
constexpr Matrix4x4Impl<T>::Matrix4x4Impl(std::span<T> src)
{
    assert(src.size() == 16 && "Invalid span size.");
    std::copy(src.begin(), src.end(), data.begin());
}

template <FloatingType T>
template <typename... Ts> requires ((std::is_convertible_v<Ts, T> && ...) && sizeof...(Ts) == 16)
constexpr Matrix4x4Impl<T>::Matrix4x4Impl(Ts... values)
    : data{ static_cast<T>(values)... }
{
}

template <FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::Identity()
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 4; ++i)
    {
        ret[i, i] = T{ 1 };
    }
    return ret;
}

template <FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::Zero()
{
    return Matrix4x4Impl{};
}

template <FloatingType T>
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

template <FloatingType T>
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
    const T abs_determinant = determinant < T{ 0 } ? -determinant : determinant;
    if (abs_determinant < KINDA_SMALL_NUMBER) // !std::isfinite(determinant)
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

template <FloatingType T>
T* Matrix4x4Impl<T>::GetData() noexcept
{
    return data.data();
}

template <FloatingType T>
const T* Matrix4x4Impl<T>::GetData() const noexcept
{
    return data.data();
}

template <FloatingType T>
auto Matrix4x4Impl<T>::GetView() noexcept
{
    return std::mdspan<T, ExtentType>(data.data());
}

template <FloatingType T>
auto Matrix4x4Impl<T>::GetView() const noexcept
{
    return std::mdspan<const T, ExtentType>(data.data());
}

template <FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::operator+(const Matrix4x4Impl& rhs) const
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 16; ++i)
    {
        ret.data[i] = data[i] + rhs.data[i];
    }
    return ret;
}

template <FloatingType T>
constexpr Matrix4x4Impl<T>& Matrix4x4Impl<T>::operator+=(const Matrix4x4Impl& rhs)
{
    for (SizeType i = 0; i < 16; ++i)
    {
        data[i] += rhs.data[i];
    }
    return *this;
}

template <FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::operator*(const Matrix4x4Impl& rhs) const
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 4; ++i)
    {
        for (SizeType j = 0; j < 4; ++j)
        {
            for (SizeType k = 0; k < 4; ++k)
            {
                ret[i, j] += (*this)[i, k] * rhs[k, j];
            }
        }
    }
    return ret;
}

template <FloatingType T>
constexpr Matrix4x4Impl<T> Matrix4x4Impl<T>::operator*(T scalar) const
{
    Matrix4x4Impl ret{};
    for (auto [n, ret_data] : ret.data | std::views::enumerate)
    {
        ret_data = data[n] * scalar;
    }
    return ret;
}

template <FloatingType T>
constexpr void Matrix4x4Impl<T>::operator*=(const Matrix4x4Impl& rhs)
{
    *this = *this * rhs;
}

template <FloatingType T>
constexpr void Matrix4x4Impl<T>::operator*=(T scalar)
{
    *this = *this * scalar;
}

template <FloatingType T>
constexpr T& Matrix4x4Impl<T>::operator[](SizeType row, SizeType col) noexcept
{
    return data[row * 4 + col];
}

template <FloatingType T>
constexpr T Matrix4x4Impl<T>::operator[](SizeType row, SizeType col) const noexcept
{
    return data[row * 4 + col];
}
}
