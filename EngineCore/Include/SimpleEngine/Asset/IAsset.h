#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::asset
{
/**
 * Asset을 나타내는 가장 기본적인 타입
 * @todo C++26때 IAsset을 상속받는 모든 타입을 Custom Annotation으로 변경
 */
struct SE_CORE_API IAsset {};
}
