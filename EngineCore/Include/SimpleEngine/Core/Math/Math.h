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
using Matrix4x4 = math::Matrix4x4Impl<f64>;
using Matrix4x4f = math::Matrix4x4Impl<f32>;

// Vector
using Vector2 = math::Vector2Impl<f64>;
using Vector2f = math::Vector2Impl<f32>;
using Vector3 = math::Vector3Impl<f64>;
using Vector3f = math::Vector3Impl<f32>;
using Vector4 = math::Vector4Impl<f64>;
using Vector4f = math::Vector4Impl<f32>;

// Quaternion
using Quaternion = math::QuaternionImpl<f64>;
using Quaternionf = math::QuaternionImpl<f32>;

// Rotator
using Rotator = math::RotatorImpl<f64>;
using Rotatorf = math::RotatorImpl<f32>;

// Geometry & Colors
using AABB = math::AABBImpl<f64>;
using AABBf = math::AABBImpl<f32>;
using Ray = math::RayImpl<f64>;
using Rayf = math::RayImpl<f32>;
using Color = math::Color;
using LinearColor = math::LinearColor;
} // namespace se
