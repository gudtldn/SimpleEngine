#pragma once

#include "SimpleEngine/Core/Math/AABB.h"
#include "SimpleEngine/Core/Math/Color.h"
#include "SimpleEngine/Core/Math/MathLiterals.h"
#include "SimpleEngine/Core/Math/MathUtility.h"
#include "SimpleEngine/Core/Math/Matrix.h"
#include "SimpleEngine/Core/Math/Ray.h"
#include "SimpleEngine/Core/Math/RotationTypes.h"
#include "SimpleEngine/Core/Math/TransformUtility.h"
#include "SimpleEngine/Core/Math/Vector2.h"
#include "SimpleEngine/Core/Math/Vector3.h"
#include "SimpleEngine/Core/Math/Vector4.h"


namespace se
{
// Matrix
using Matrix4x4 = math::Matrix4x4Impl<double>;
using Matrix4x4f = math::Matrix4x4Impl<float>;

// Vector
using Vector2 = math::Vector2Impl<double>;
using Vector2f = math::Vector2Impl<float>;
using Vector3 = math::Vector3Impl<double>;
using Vector3f = math::Vector3Impl<float>;
using Vector4 = math::Vector4Impl<double>;
using Vector4f = math::Vector4Impl<float>;

// Quaternion
using Quaternion = math::QuaternionImpl<double>;
using Quaternionf = math::QuaternionImpl<float>;

// Rotator
using Rotator = math::RotatorImpl<double>;
using Rotatorf = math::RotatorImpl<float>;

// Geometry & Colors
using AABB = math::AABBImpl<double>;
using AABBf = math::AABBImpl<float>;
using Ray = math::RayImpl<double>;
using Rayf = math::RayImpl<float>;
using Color = math::Color;
using LinearColor = math::LinearColor;
}  // namespace se
