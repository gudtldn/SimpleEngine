#pragma once
#include <concepts>
#include <filesystem>
#include <memory>

#include "SimpleEngine/Core/Concurrency/Coroutine.h"


namespace se::asset
{
template <typename Signature>
class AssetLoader;

/**
 * AssetLoader가 구현해야할 필수 내용
 * @todo 추후 어떻게 사용하고, 어떻게 구현해야 하는지 주석 작성
 */
template <typename AssetType>
concept AssetLoadable = requires(AssetLoader<AssetType> loader, const std::filesystem::path& path)
{
    { loader.Load(path) } -> std::same_as<core::concurrency::Task<std::shared_ptr<AssetType>>>;
};
}
