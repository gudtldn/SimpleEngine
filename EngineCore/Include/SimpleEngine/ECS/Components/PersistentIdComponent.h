#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"


namespace se
{
/**
 * 월드 파일에서 엔티티를 가리키는 영속 ID를 담는 컴포넌트
 * WorldFile이 처음 저장할 때 붙이고, 읽을 때 파일의 ID로 되살립니다. 0은 아직 ID가 없다는 뜻입니다.
 */
struct SE_CORE_API PersistentIdComponent
{
    SE_ANNOTATE(id, display::ReadOnly)
    u64 id = 0;
};
} // namespace se

SE_DECLARE_REFLECTION(se::PersistentIdComponent, SE_CORE_API)
