#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/**
 * Entity가 사용할 재질(Material) 리소스의 ID를 지정하는 컴포넌트
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Component) MeshMaterialComponent
{
    // placeholder: Material 시스템 구현 전까지 비어있음
};
} // namespace se

SE_DECLARE_REFLECTION(se::MeshMaterialComponent)
