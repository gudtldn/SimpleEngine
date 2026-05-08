#pragma once

#include "SimpleEngine/Traits/TypeTraits.h"


namespace se::math
{
template <traits::FloatingType T>
struct Matrix4x4Impl;

template <traits::FloatingType T>
struct RotatorImpl;

template <traits::FloatingType T>
struct QuaternionImpl;

template <traits::FloatingType T>
struct Vector2Impl;

template <traits::FloatingType T>
struct Vector3Impl;

template <traits::FloatingType T>
struct Vector4Impl;

template <traits::FloatingType T>
struct AABBImpl;

template <traits::FloatingType T>
struct RayImpl;

struct Color;
struct LinearColor;
} // namespace se::math
