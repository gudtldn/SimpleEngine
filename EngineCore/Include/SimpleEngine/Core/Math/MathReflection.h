#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"


// 컴포넌트와 에셋의 필드로 쓰는 수학 타입의 새 리플렉션 등록. 등록 블록과 트레이트는 MathReflection.cpp에 있음
SE_DECLARE_REFLECTION(se::Vector3, SE_CORE_API)
SE_DECLARE_REFLECTION(se::Quaternion, SE_CORE_API)

// 각도는 숫자 하나로 쓰도록 Opaque로 등록하고, 직렬화는 MathReflection.cpp의 SerializeTraits가 맡음
SE_REFLECT_OPAQUE(se::Degree<f64>)
