#pragma once
#include <cassert>

#include "SimpleEngine/Core/Math/MathFwd.h"


namespace se::math
{
/**
 * @todo docs
 */
template <traits::FloatingType T>
struct RayImpl
{
    using VectorType = Vector3Impl<T>;

    VectorType origin;
    VectorType direction;

public:
    constexpr RayImpl()
        : origin(VectorType::Zero())
        , direction(VectorType::Forward())
    {
    }

    constexpr RayImpl(const VectorType& in_origin, const VectorType& in_direction)
        : origin(in_origin)
        , direction(in_direction)
    {
        if (!direction.IsNormalized())
        {
            direction.Normalize();
        }
    }

public:
    [[nodiscard]] constexpr VectorType GetPoint(T distance) const
    {
        return origin + (direction * distance);
    }

    /**
     * AABB와의 교차 여부를 검사합니다. (Slab Method)
     * @param in_aabb 검사할 AABB
     * @param out_distance 교차 발생 시 진입점까지의 거리 (Ray Origin이 내부라면 음수가 나올 수 있음)
     * @return 교차하면 true
     */
    [[nodiscard]] constexpr bool Intersects(const AABBImpl<T>& in_aabb, T& out_distance) const
    {
        // 방향의 역수를 미리 계산
        const VectorType inv_dir = VectorType{ static_cast<T>(1) } / direction;

        // X축 Slab 검사
        T t1 = (in_aabb.min.x - origin.x) * inv_dir.x;
        T t2 = (in_aabb.max.x - origin.x) * inv_dir.x;

        // t_min: 진입점 중 가장 늦은(큰) 값
        // t_max: 탈출점 중 가장 빠른(작은) 값
        T t_min = Min(t1, t2);
        T t_max = Max(t1, t2);

        // Y축 Slab 검사
        t1 = (in_aabb.min.y - origin.y) * inv_dir.y;
        t2 = (in_aabb.max.y - origin.y) * inv_dir.y;

        t_min = Max(t_min, Min(t1, t2));
        t_max = Min(t_max, Max(t1, t2));

        // Z축 Slab 검사
        t1 = (in_aabb.min.z - origin.z) * inv_dir.z;
        t2 = (in_aabb.max.z - origin.z) * inv_dir.z;

        t_min = Max(t_min, Min(t1, t2));
        t_max = Min(t_max, Max(t1, t2));

        out_distance = t_min;

        // 충돌 조건
        // 1. t_max >= t_min: 교차 구간이 유효함 (겹치는 구간이 존재)
        // 2. t_max >= 0: 상자가 레이의 뒤쪽에 있지 않음
        return t_max >= t_min && t_max >= static_cast<T>(0);
    }

    /** AABB와의 교차 여부를 반환합니다. */
    [[nodiscard]] constexpr bool Intersects(const AABBImpl<T>& in_aabb) const
    {
        T dist;
        return Intersects(in_aabb, dist);
    }
};
}  // namespace se::math
