#pragma once
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Serialization/Transient.h"


namespace se
{
/**
 * 3D 공간에서 Entity의 위치, 회전, 크기를 정의하는 컴포넌트
 */
struct SE_CORE_API TransformComponent
{
    Quaternion rotation = Quaternion::Identity();

    Vector3 position = Vector3::Zero();

    Vector3 scale = Vector3::One();

    // PropagateTransforms에서 최적화용 flag로 사용
    SE_ANNOTATE(dirty, serde::Transient, display::Hidden)
    bool dirty = true;
};
} // namespace se

SE_DECLARE_REFLECTION(se::TransformComponent, SE_CORE_API)
