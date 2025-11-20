#pragma once
#include "SimpleEngine/Asset/ImportSettings/IAssetImportSettings.h"


namespace se::asset
{
/**
 * @todo docs
 */
class SE_CORE_API TextureImportSettings : public IAssetImportSettings
{
public:
    bool is_srgb = true;       // Color(true) vs Data(false)
    bool generate_mips = true; // 밉맵 생성 여부
};
}
