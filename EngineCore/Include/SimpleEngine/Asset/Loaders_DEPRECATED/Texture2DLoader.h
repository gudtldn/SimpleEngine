#pragma once
#include "SimpleEngine/Asset/Loaders_DEPRECATED/IAssetLoader.h"


namespace se::asset
{
class SE_CORE_API Texture2DLoader : public IAssetLoader
{
public:
    virtual concurrency::Task<std::shared_ptr<IAsset>> Load(
        const std::filesystem::path& physical_path,
        const IAssetImportSettings* import_settings
    ) override;
};
}
