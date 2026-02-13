#pragma once
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


/**
 * 수학 타입의 ADL Serialize 함수 모음
 */
namespace se::math
{
// Angle Types (Degree, Radian)
template <traits::FloatingType T, typename Tag>
void Serialize(Archive& ar, AngleType<T, Tag>& angle)
{
    ar("value") << angle.value;
}

// Vector Types
template <traits::FloatingType T>
void Serialize(Archive& ar, Vector2Impl<T>& v)
{
    ar("x") << v.x;
    ar("y") << v.y;
}

template <traits::FloatingType T>
void Serialize(Archive& ar, Vector3Impl<T>& v)
{
    ar("x") << v.x;
    ar("y") << v.y;
    ar("z") << v.z;
}

template <traits::FloatingType T>
void Serialize(Archive& ar, Vector4Impl<T>& v)
{
    ar("x") << v.x;
    ar("y") << v.y;
    ar("z") << v.z;
    ar("w") << v.w;
}

// Rotation Types
template <traits::FloatingType T>
void Serialize(Archive& ar, QuaternionImpl<T>& q)
{
    ar("x") << q.x;
    ar("y") << q.y;
    ar("z") << q.z;
    ar("w") << q.w;
}

template <traits::FloatingType T>
void Serialize(Archive& ar, RotatorImpl<T>& r)
{
    ar("pitch") << r.pitch;
    ar("yaw") << r.yaw;
    ar("roll") << r.roll;
}

// Geometry Types
template <traits::FloatingType T>
void Serialize(Archive& ar, AABBImpl<T>& aabb)
{
    ar("min") << aabb.min;
    ar("max") << aabb.max;
}

template <traits::FloatingType T>
void Serialize(Archive& ar, RayImpl<T>& ray)
{
    ar("origin") << ray.origin;
    ar("direction") << ray.direction;
}

// Color Types
inline void Serialize(Archive& ar, LinearColor& c)
{
    ar("r") << c.r;
    ar("g") << c.g;
    ar("b") << c.b;
    ar("a") << c.a;
}

inline void Serialize(Archive& ar, Color& c)
{
    ar("r") << c.r;
    ar("g") << c.g;
    ar("b") << c.b;
    ar("a") << c.a;
}
}  // namespace se::math
