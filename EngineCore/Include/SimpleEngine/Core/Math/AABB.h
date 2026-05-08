#pragma once

#include "SimpleEngine/Core/Math/Vector3.h"
#include "SimpleEngine/Core/Math/MathUtility.h"

#include <limits>


namespace se::math
{
/**
 * Axis-Aligned Bounding Box를 나타내는 구조체
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

    template <traits::FloatingType U>
    explicit(sizeof(U) > sizeof(T))
    constexpr AABBImpl(const AABBImpl<U>& other)
        : min(static_cast<Vector3Impl<T>>(other.min))
        , max(static_cast<Vector3Impl<T>>(other.max))
    {
    }

    /** 중심점(Center)과 각 축의 절반 크기(Extent)를 사용하여 생성합니다. */
    static constexpr AABBImpl FromCenterExtent(const VectorType& center, const VectorType& extent)
    {
        return AABBImpl{
            center - extent,
            center + extent
        };
    }

    /** 중심점(Center)과 균일한 절반 크기(Extent)를 사용하여 정육면체 형태를 생성합니다. */
    static constexpr AABBImpl FromCenterExtent(const VectorType& center, T extent)
    {
        const VectorType v_ext{ extent };
        return AABBImpl{
            center - v_ext,
            center + v_ext
        };
    }

public:
    /** AABB의 중심점을 반환합니다. */
    [[nodiscard]] constexpr VectorType GetCenter() const
    {
        return (min + max) * 0.5;
    }

    /** 중심에서 면까지의 거리를 반환합니다. */
    [[nodiscard]] constexpr VectorType GetExtent() const
    {
        return (max - min) * 0.5;
    }

    /** AABB의 전체 크기(가로, 세로, 높이)를 반환합니다. */
    [[nodiscard]] constexpr VectorType GetSize() const
    {
        return max - min;
    }

    /** 현재 AABB가 유효한지(min <= max) 확인합니다. */
    [[nodiscard]] constexpr bool IsValid() const
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    /**
     * 주어진 점을 포함하도록 AABB를 확장합니다.
     * @return 확장 성공 여부
     */
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

    /**
     * 다른 AABB를 포함하도록 영역을 확장합니다.
     * @param other 병합할 다른 AABB
     * @return other가 유효하지 않으면 false, 병합에 성공하면 true
     */
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

    /** 점이 AABB 내부에 포함되는지 확인합니다. */
    [[nodiscard]] constexpr bool Contains(const VectorType& point) const
    {
        return point.x >= min.x && point.x <= max.x
            && point.y >= min.y && point.y <= max.y
            && point.z >= min.z && point.z <= max.z;
    }

    /** 다른 AABB와 겹치는지(교차하는지) 확인합니다. */
    [[nodiscard]] constexpr bool Intersects(const AABBImpl& other) const
    {
        return min.x <= other.max.x && max.x >= other.min.x
            && min.y <= other.max.y && max.y >= other.min.y
            && min.z <= other.max.z && max.z >= other.min.z;
    }
};
} // namespace se::math
