#pragma once

#include <limits>

#include "SimpleEngine/Core/Math/Vector3.h"
#include "SimpleEngine/Core/Math/MathUtility.h"


namespace se::math
{
/**
 * @todo docs
 */
template <traits::FloatingType T>
struct AABBImpl
{
    using VectorType = Vector3Impl<T>;

    VectorType min;
    VectorType max;

public:
    constexpr AABBImpl()
        : min(VectorType{  std::numeric_limits<T>::infinity() })
        , max(VectorType{ -std::numeric_limits<T>::infinity() })
    {
    }

    constexpr AABBImpl(const VectorType& in_min, const VectorType& in_max)
        : min(in_min)
        , max(in_max)
    {
    }

    static constexpr AABBImpl FromCenterExtent(const VectorType& center, const VectorType& extent)
    {
        return AABBImpl{
            center - extent,
            center + extent
        };
    }

    static constexpr AABBImpl FromCenterExtent(const VectorType& center, T extent)
    {
        const VectorType v_ext{ extent };
        return AABBImpl{
            center - v_ext,
            center + v_ext
        };
    }

public:
    [[nodiscard]] constexpr VectorType GetCenter() const
    {
        return (min + max) * 0.5;
    }

    [[nodiscard]] constexpr VectorType GetExtent() const
    {
        return (max - min) * 0.5;
    }

    [[nodiscard]] constexpr VectorType GetSize() const
    {
        return max - min;
    }

    [[nodiscard]] constexpr bool IsValid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    constexpr bool Expand(const VectorType& point)
    {
        min.x = Min(min.x, point.x);
        min.y = Min(min.y, point.y);
        min.z = Min(min.z, point.z);

        max.x = Max(max.x, point.x);
        max.y = Max(max.y, point.y);
        max.z = Max(max.z, point.z);

        return true;
    }

    constexpr bool Expand(const AABBImpl& other)
    {
        if (!other.IsValid())
        {
            return false;
        }

        min.x = Min(min.x, other.min.x);
        min.y = Min(min.y, other.min.y);
        min.z = Min(min.z, other.min.z);

        max.x = Max(max.x, other.max.x);
        max.y = Max(max.y, other.max.y);
        max.z = Max(max.z, other.max.z);

        return true;
    }

    [[nodiscard]] constexpr bool Contains(const VectorType& point) const
    {
        return point.x >= min.x && point.x <= max.x
            && point.y >= min.y && point.y <= max.y
            && point.z >= min.z && point.z <= max.z;
    }

    [[nodiscard]] constexpr bool Intersects(const AABBImpl& other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x
            && min.y <= other.max.y && max.y >= other.min.y
            && min.z <= other.max.z && max.z >= other.min.z;
    }
};
}  // namespace se::math
