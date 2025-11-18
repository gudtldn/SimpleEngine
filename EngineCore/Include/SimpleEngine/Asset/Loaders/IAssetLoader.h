#pragma once
#include <filesystem>
#include <memory>

#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Core/Concurrency/Coroutine.h"


namespace se::asset
{
class SE_CORE_API IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;
    virtual concurrency::Task<std::shared_ptr<IAsset>> Load(const std::filesystem::path& physical_path) = 0;
};
}
