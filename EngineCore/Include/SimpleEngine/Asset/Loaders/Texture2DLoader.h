#pragma once
#include "SimpleEngine/Asset/Loaders/IAssetLoader.h"


namespace se::asset
{
class SE_CORE_API Texture2DLoader : public IAssetLoader
{
public:
    virtual concurrency::Task<std::shared_ptr<IAsset>> Load(const std::filesystem::path& physical_path) override;
};
}
