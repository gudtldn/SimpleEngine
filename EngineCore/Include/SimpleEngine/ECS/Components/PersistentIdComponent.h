#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "../../Core/Reflection/Legacy/Annotations.h"


namespace se
{
/**
 * 월드 파일에서 엔티티를 가리키는 영속 ID를 담는 컴포넌트
 * WorldFileWriter가 처음 저장할 때 붙이고, WorldFileReader가 파일의 ID로 되살립니다. 0은 아직 ID가 없다는 뜻입니다.
 */
struct SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Transient, =meta::Component) PersistentIdComponent
{
    SE_ANNOTATION(=meta::Reflect, =meta::ReadOnly)
    u64 id = 0;
};
} // namespace se

SE_DECLARE_REFLECTION_V1(se::PersistentIdComponent)
SE_DECLARE_REFLECTION(se::PersistentIdComponent, SE_CORE_API)
