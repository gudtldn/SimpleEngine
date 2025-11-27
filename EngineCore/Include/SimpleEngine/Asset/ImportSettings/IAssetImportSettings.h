#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::asset
{
/**
 * AssetImportSettings의 기본 인터페이스
 * 모든 AssetImportSettings 클래스는 이를 상속받아야 합니다.
 */
class SE_CORE_API IAssetImportSettings
{
public:
    virtual ~IAssetImportSettings() = default;
};
}
