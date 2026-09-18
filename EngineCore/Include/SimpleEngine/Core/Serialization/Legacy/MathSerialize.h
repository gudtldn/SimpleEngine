#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Serialization/Legacy/Archive.h"


/**
 * 수학 타입의 ADL Serialize 함수 모음
 */
namespace se::math
{
// Angle Types (Degree, Radian)
template <traits::FloatingType T, typename Tag>
void Serialize(Archive_v1& ar, AngleType<T, Tag>& angle)
{
    ar("value") << angle.value;
}

// Vector Types
template <traits::FloatingType T>
void Serialize(Archive_v1& ar, Vector2Impl<T>& v)
{
    ar("x") << v.x;
    ar("y") << v.y;
}

template <traits::FloatingType T>
void Serialize(Archive_v1& ar, Vector3Impl<T>& v)
{
    ar("x") << v.x;
    ar("y") << v.y;
    ar("z") << v.z;
}

template <traits::FloatingType T>
void Serialize(Archive_v1& ar, Vector4Impl<T>& v)
{
    ar("x") << v.x;
    ar("y") << v.y;
    ar("z") << v.z;
    ar("w") << v.w;
}

// Rotation Types
template <traits::FloatingType T>
void Serialize(Archive_v1& ar, QuaternionImpl<T>& q)
{
    ar("x") << q.x;
    ar("y") << q.y;
    ar("z") << q.z;
    ar("w") << q.w;
}

template <traits::FloatingType T>
void Serialize(Archive_v1& ar, RotatorImpl<T>& r)
{
    ar("pitch") << r.pitch;
    ar("yaw") << r.yaw;
    ar("roll") << r.roll;
}

// Matrix Types
template <traits::FloatingType T>
void Serialize(Archive_v1& ar, Matrix4x4Impl<T>& m)
{
    ar("data") << m.data;
}

// Geometry Types
template <traits::FloatingType T>
void Serialize(Archive_v1& ar, AABBImpl<T>& aabb)
{
    ar("min") << aabb.min;
    ar("max") << aabb.max;
}

template <traits::FloatingType T>
void Serialize(Archive_v1& ar, RayImpl<T>& ray)
{
    ar("origin") << ray.origin;
    ar("direction") << ray.direction;
}

// Color Types
inline void Serialize(Archive_v1& ar, LinearColor& c)
{
    ar("r") << c.r;
    ar("g") << c.g;
    ar("b") << c.b;
    ar("a") << c.a;
}

inline void Serialize(Archive_v1& ar, Color& c)
{
    ar("r") << c.r;
    ar("g") << c.g;
    ar("b") << c.b;
    ar("a") << c.a;
}
} // namespace se::math
