export module SimpleEngine.Math:Matrix;

import SimpleEngine.TypeTraits;
import SimpleEngine.Types;
import std;

using namespace se::type_traits;


/**
 * row-major matrix template
 */
template <FloatingType T, uint32 Rows, uint32 Cols, size_t Align = 16>
struct alignas(Align) MatrixImpl
{
private:
    using SizeType = size_t;
    using ExtentType = std::extents<SizeType, Rows, Cols>;

    std::array<T, Rows * Cols> data;

public:
    constexpr MatrixImpl() = default;

    static constexpr MatrixImpl Identity() requires (Rows == Cols)
    {
        MatrixImpl ret{};
        for (SizeType i = 0; i < Rows; ++i)
        {
            ret[i, i] = T{ 1 };
        }
        return ret;
    }

    static constexpr MatrixImpl Zero()
    {
        return MatrixImpl{};
    }

public:
    constexpr T* GetData() noexcept { return data.data(); }
    constexpr const T* GetData() const noexcept { return data.data(); }

    constexpr auto GetView() noexcept
    {
        return std::mdspan<T, ExtentType>(data.data());
    }

    constexpr auto GetView() const noexcept
    {
        return std::mdspan<const T, ExtentType>(data.data());
    }

public:
    constexpr T& operator[](SizeType row, SizeType col) noexcept
    {
        return data[row * Cols + col];
    }

    constexpr const T& operator[](SizeType row, SizeType col) const noexcept
    {
        return data[row * Cols + col];
    }
};
