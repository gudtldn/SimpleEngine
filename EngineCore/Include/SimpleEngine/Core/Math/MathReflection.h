#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"


SE_DECLARE_REFLECTION(se::Vector2, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Vector2f, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Vector3, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Vector3f, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Vector4, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Vector4f, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Quaternion, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Matrix4x4, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Matrix4x4f, SE_CORE_API)
SE_DECLARE_REFLECTION(se::AABBf, SE_CORE_API)

// 각도는 숫자 하나로 쓰도록 Opaque로 등록하고, 직렬화는 MathReflection.cpp의 SerializeTraits에서 수행
SE_REFLECT_OPAQUE(se::Degree<f64>)
