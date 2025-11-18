#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::asset
{
/**
 * Asset을 나타내는 가장 기본적인 타입
 */
class SE_CORE_API IAsset
{
public:
    virtual ~IAsset() = default;
};
}
