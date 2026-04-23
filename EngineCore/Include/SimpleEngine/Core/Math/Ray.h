#pragma once

#include "SimpleEngine/Core/Math/MathFwd.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::math
{
/**
 * 3차원 공간상의 광선(Ray)을 나타내는 구조체
 */
template <traits::FloatingType T>
struct RayImpl
{
    using VectorType = Vector3Impl<T>;

    VectorType origin;
    VectorType direction;

public:
    /** 기본 생성자: 원점(0,0,0)에서 앞쪽(+Y)으로 쏘는 광선을 생성합니다. */
    constexpr RayImpl()
        : origin(VectorType::Zero())
        , direction(VectorType::Forward())
    {
    }

    /**
     * 시작점과 방향으로 Ray를 생성합니다.
     * @param in_origin 시작점
     * @param in_direction 방향 벡터 (길이가 1이어야 함)
     */
    constexpr RayImpl(const VectorType& in_origin, const VectorType& in_direction)
        : origin(in_origin)
        , direction(in_direction)
    {
        SE_ASSERT(direction.IsNormalized(), "Ray direction must be normalized.");
    }

    template <traits::FloatingType U>
    explicit(sizeof(U) > sizeof(T))
    constexpr RayImpl(const RayImpl<U>& other)
        : origin(static_cast<Vector3Impl<T>>(other.origin))
        , direction(static_cast<Vector3Impl<T>>(other.direction))
    {
    }

public:
    /**
     * 원점으로부터 주어진 거리만큼 이동한 위치의 좌표를 구합니다.
     * @param distance 거리
     * @return Point (origin + (direction * distance))
     */
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

        // Ray를 무한한 직선이라고 가정하고, 각 축(Slab)을 통과할 때마다 "유효한 교차 구간"을 점점 좁혀 나간다.
        // t_min: 교차 구간의 진입점 (점점 뒤로 밀림 -> Max)
        // t_max: 교차 구간의 탈출점 (점점 앞으로 당겨짐 -> Min)
        T t_min = -std::numeric_limits<T>::infinity();
        T t_max = std::numeric_limits<T>::infinity();

        // X축 Slab 검사
        // t1, t2: 현재 축의 평면(min, max)에 도달하는 거리
        T t1 = (in_aabb.min.x - origin.x) * inv_dir.x;
        T t2 = (in_aabb.max.x - origin.x) * inv_dir.x;

        // 광선이 역방향(-x)으로 올 수도 있으므로,
        // 더 작은 값이 진입점(entry), 더 큰 값이 탈출점(exit)이 됨
        t_min = Max(t_min, Min(t1, t2));
        t_max = Min(t_max, Max(t1, t2));

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

        // 1. t_max < t_min:
        //    유효 구간이 존재하지 않음. (진입하기도 전에 다른 축에서 이미 탈출해버림 = 빗나감)
        // 2. t_max < 0:
        //    교차 구간은 있지만, Ray의 반대편(뒤쪽)에 있음.
        if (t_max < t_min || t_max < T(0))
        {
            return false;
        }

        // t_min이 양수면: 박스 밖에서 쏨 -> 첫 진입점(t_min) 반환
        // t_min이 음수면: 박스 안에서 쏨 -> 탈출점(t_max) 반환 (뚫고 나가는 거리)
        out_distance = (t_min >= T(0)) ? t_min : t_max;
        return true;
    }

    /** AABB와의 교차 여부를 반환합니다. */
    [[nodiscard]] constexpr bool Intersects(const AABBImpl<T>& in_aabb) const
    {
        T dist;
        return Intersects(in_aabb, dist);
    }

    /**
     * 마우스 Ray와 특정 3D 평면의 교차점(Intersection)을 계산합니다.
     *
     * 평면은 plane_origin을 지나고 plane_normal을 법선으로 가지는 무한 평면입니다.
     * t = dot(plane_origin - origin, plane_normal) / dot(direction, plane_normal)
     *
     * @param plane_origin 평면 위 임의의 점 (기즈모 중심)
     * @param plane_normal 평면 법선 단위벡터
     * @param out_distance  (ray.origin + t * ray.direction). t < 0이면 false 반환
     * @return 교차 여부 (Ray-평면이 평행하거나, 카메라 뒤쪽이면 false)
     */
    [[nodiscard]] constexpr bool IntersectPlane(
        const VectorType& plane_origin, const VectorType& plane_normal, T& out_distance) const
    {
        const T denom = direction.Dot(plane_normal);
        if (Abs(denom) < static_cast<T>(KINDA_SMALL_NUMBER))
        {
            return false;
        }

        out_distance = (plane_origin - origin).Dot(plane_normal) / denom;
        return out_distance >= T(0);
    }

    /**
     * 공간상의 두 직선(마우스 Ray와 기즈모 축) 사이의 최근접 거리를 계산합니다. (Skew Lines 알고리즘)
     * @see https://en.wikipedia.org/wiki/Skew_lines#Nearest_points
     * @param line_origin 직선의 기준점 (기즈모 중심)
     * @param line_dir 직선 방향 단위벡터
     * @return 직선 상의 파라미터 값 t (line_origin + t * line_dir)
     */
    [[nodiscard]] constexpr T ClosestParameterOnLine(const VectorType& line_origin, const VectorType& line_dir) const
    {
        const VectorType w = origin - line_origin;
        const T a = direction.Dot(direction); // |direction|^2 = 1 (정규화됨)
        const T b = direction.Dot(line_dir);
        const T c = line_dir.Dot(line_dir); // |line_dir|^2 = 1 (정규화됨)
        const T d = direction.Dot(w);
        const T e = line_dir.Dot(w);

        const T denom = a * c - b * b;

        // 두 직선이 완전히 평행하여 교차점 근사치가 무한대로 발산하는 것을 방지
        if (Abs(denom) < static_cast<T>(KINDA_SMALL_NUMBER))
        {
            return T(0);
        }

        // 마우스 Ray가 기즈모 축에 투영된 최단 위치(t)를 반환
        return (a * e - b * d) / denom;
    }
};
}  // namespace se::math
