export module SimpleEngine.Math:Matrix;
import :MathUtility;
import :MathLiterals;
import :RotationTypes;
import :Vector3;

import SimpleEngine.Traits;
import SimpleEngine.Types;
import std;

import <cassert>;

using namespace se::traits::type_traits;


/**
 * row-major matrix template
 */
template <FloatingType T, size_t Align = 16>
struct alignas(Align) Matrix4x4Impl
{
private:
    std::array<T, 16> data;

public:
    using RealType = T;
    using SizeType = size_t;
    using ExtentType = std::extents<SizeType, 4, 4>;

public:
    constexpr Matrix4x4Impl() = default;
    constexpr Matrix4x4Impl(std::span<const T, 16> src);
    template <typename... Ts>
        requires ((std::is_convertible_v<Ts, T> && ...) && sizeof...(Ts) == 16)
    constexpr Matrix4x4Impl(Ts... values);

    [[nodiscard]] static constexpr Matrix4x4Impl Identity();
    [[nodiscard]] static constexpr Matrix4x4Impl Zero();

    [[nodiscard]] static constexpr Matrix4x4Impl MakeFromTranslation(const Vector3Impl<T>& translation);
    [[nodiscard]] static constexpr Matrix4x4Impl MakeFromRotation(const RotatorImpl<T>& rotation);
    [[nodiscard]] static constexpr Matrix4x4Impl MakeFromRotation(const QuaternionImpl<T>& quaternion);
    [[nodiscard]] static constexpr Matrix4x4Impl MakeFromScale(const Vector3Impl<T>& scale);

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
};

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align>::Matrix4x4Impl(std::span<const T, 16> src)
{
    std::copy(src.begin(), src.end(), data.begin());
}

template <FloatingType T, size_t Align>
template <typename... Ts> requires ((std::is_convertible_v<Ts, T> && ...) && sizeof...(Ts) == 16)
constexpr Matrix4x4Impl<T, Align>::Matrix4x4Impl(Ts... values)
    : data{ static_cast<T>(values)... }
{
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::Identity()
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 4; ++i)
    {
        ret[i, i] = T{ 1 };
    }
    return ret;
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::Zero()
{
    return Matrix4x4Impl{};
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::MakeFromTranslation(const Vector3Impl<T>& translation)
{
    T x = translation.x;
    T y = translation.y;
    T z = translation.z;

    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1
    };
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::MakeFromRotation(const RotatorImpl<T>& rotation)
{
    const Radian<T> pitch_rad{ rotation.pitch };
    const Radian<T> yaw_rad{ rotation.yaw };
    const Radian<T> roll_rad{ rotation.roll };

    const T sin_p = MathUtils::Sin(pitch_rad), cos_p = MathUtils::Cos(pitch_rad);
    const T sin_y = MathUtils::Sin(yaw_rad), cos_y = MathUtils::Cos(yaw_rad);
    const T sin_r = MathUtils::Sin(roll_rad), cos_r = MathUtils::Cos(roll_rad);

    // Rz(yaw)
    Matrix4x4Impl rz{
        cos_y, -sin_y, 0, 0,
        sin_y, cos_y, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    // Rx(pitch)
    Matrix4x4Impl rx{
        1, 0, 0, 0,
        0, cos_p, -sin_p, 0,
        0, sin_p, cos_p, 0,
        0, 0, 0, 1
    };

    // Ry(roll)
    Matrix4x4Impl ry{
        cos_r, 0, sin_r, 0,
        0, 1, 0, 0,
        -sin_r, 0, cos_r, 0,
        0, 0, 0, 1
    };

    // Rz(yaw) * Rx(pitch) * Ry(roll)
    return rz * rx * ry;
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::MakeFromRotation(const QuaternionImpl<T>& quaternion)
{
    const T x = quaternion.x;
    const T y = quaternion.y;
    const T z = quaternion.z;
    const T w = quaternion.w;

    const T xx = x * x;
    const T yy = y * y;
    const T zz = z * z;
    const T xy = x * y;
    const T xz = x * z;
    const T yz = y * z;
    const T wx = w * x;
    const T wy = w * y;
    const T wz = w * z;

    // Row-major 3x3 rotation block for row-vector (p' = p M)
    return {
        1 - 2 * (yy + zz), 2 * (xy - wz), 2 * (xz + wy), 0,
        2 * (xy + wz), 1 - 2 * (xx + zz), 2 * (yz - wx), 0,
        2 * (xz - wy), 2 * (yz + wx), 1 - 2 * (xx + yy), 0,
        0, 0, 0, 1
    };
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::MakeFromScale(const Vector3Impl<T>& scale)
{
    T x = scale.x;
    T y = scale.y;
    T z = scale.z;

    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1
    };
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::Transpose() const
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

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::Inverse() const
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
    if (abs_determinant < se::math::KINDA_SMALL_NUMBER) // !std::isfinite(determinant)
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

template <FloatingType T, size_t Align>
T* Matrix4x4Impl<T, Align>::GetData() noexcept
{
    return data.data();
}

template <FloatingType T, size_t Align>
const T* Matrix4x4Impl<T, Align>::GetData() const noexcept
{
    return data.data();
}

template <FloatingType T, size_t Align>
auto Matrix4x4Impl<T, Align>::GetView() noexcept
{
    return std::mdspan<T, ExtentType>(data.data());
}

template <FloatingType T, size_t Align>
auto Matrix4x4Impl<T, Align>::GetView() const noexcept
{
    return std::mdspan<const T, ExtentType>(data.data());
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::operator+(const Matrix4x4Impl& rhs) const
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 16; ++i)
    {
        ret.data[i] = data[i] + rhs.data[i];
    }
    return ret;
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align>& Matrix4x4Impl<T, Align>::operator+=(const Matrix4x4Impl& rhs)
{
    for (SizeType i = 0; i < 16; ++i)
    {
        data[i] += rhs.data[i];
    }
    return *this;
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::operator*(const Matrix4x4Impl& rhs) const
{
    Matrix4x4Impl ret{};
    for (SizeType i = 0; i < 4; ++i)
    {
        for (SizeType j = 0; j < 4; ++j)
        {
            T sum{};
            for (SizeType k = 0; k < 4; ++k)
            {
                sum += (*this)[i, k] * rhs[k, j];
            }
            ret[i, j] = sum;
        }
    }
    return ret;
}

template <FloatingType T, size_t Align>
constexpr Matrix4x4Impl<T, Align> Matrix4x4Impl<T, Align>::operator*(T scalar) const
{
    Matrix4x4Impl ret{};
    for (auto [n, ret_data] : ret.data | std::views::enumerate)
    {
        ret_data = data[n] * scalar;
    }
    return ret;
}

template <FloatingType T, size_t Align>
constexpr void Matrix4x4Impl<T, Align>::operator*=(const Matrix4x4Impl& rhs)
{
    *this = *this * rhs;
}

template <FloatingType T, size_t Align>
constexpr void Matrix4x4Impl<T, Align>::operator*=(T scalar)
{
    *this = *this * scalar;
}

template <FloatingType T, size_t Align>
constexpr T& Matrix4x4Impl<T, Align>::operator[](SizeType row, SizeType col) noexcept
{
    return data[row * 4 + col];
}

template <FloatingType T, size_t Align>
constexpr T Matrix4x4Impl<T, Align>::operator[](SizeType row, SizeType col) const noexcept
{
    return data[row * 4 + col];
}
