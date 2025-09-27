export module SE.Assets:Loaders.AssetLoader;

import SE.Core;
import SE.Types;
import std;

using namespace se::core::concurrency::coroutine;


export namespace se::assets::loaders
{
template <typename Signature>
class AssetLoader;


/**
 * AssetLoader가 구현해야할 필수 내용
 * @todo 추후 어떻게 사용하고, 어떻게 구현해야 하는지 주석 작성
 */
template <typename AssetType>
concept AssetLoadable = requires(AssetLoader<AssetType> loader, const VPath& path)
{
    { loader.Load(path) } -> std::same_as<Task<std::shared_ptr<AssetType>>>;
};
}
