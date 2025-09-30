module;
#include "tracy/Tracy.hpp"
export module SE.Assets:AssetStorage;
import :Loaders;

import SE.Core;
import SE.Types;
import std;

using namespace se::core::concurrency;
using namespace se::core::concurrency::coroutine;


namespace se::assets
{
/**
 * AssetStorage를 관리하기 위한 인터페이스
 */
class IAssetStorage
{
public:
    virtual ~IAssetStorage() = default;
    virtual void RemoveReference(const StringName& asset_id) = 0;
};

/**
 * Asset을 관리하는 클래스
 * @tparam T 관리하는 Asset의 타입
 */
template <typename T>
class AssetStorage : public IAssetStorage, public std::enable_shared_from_this<AssetStorage<T>>
{
public:
    using AssetType = T;

public:
    /** 캐시에서 에셋을 찾습니다. 없으면 nullptr를 반환합니다. */
    std::shared_ptr<T> Find(const StringName& asset_id);

    /** 캐시에 에셋을 추가합니다. 이미 존재하면 덮어씁니다. */
    void Add(const StringName& asset_id, std::shared_ptr<T> asset);

    /** Asset의 참조 카운트를 확인 후, 참조가 1개 이하일 때 Asset을 Storage에서 제거합니다. */
    virtual void RemoveReference(const StringName& asset_id) override;

private:
    TracySharedLockable(std::shared_mutex, mutex);
    unordered_map<StringName, std::shared_ptr<T>> assets;
};

template <typename T>
std::shared_ptr<T> AssetStorage<T>::Find(const StringName& asset_id)
{
    std::shared_lock lock(mutex);
    if (auto it = assets.find(asset_id); it != assets.end())
    {
        return it->second;
    }
    return nullptr;
}

template <typename T>
void AssetStorage<T>::Add(const StringName& asset_id, std::shared_ptr<T> asset)
{
    std::unique_lock lock(mutex);
    assets[asset_id] = asset;
}

template <typename T>
void AssetStorage<T>::RemoveReference(const StringName& asset_id)
{
    std::unique_lock lock(mutex);
    if (auto it = assets.find(asset_id); it != assets.end())
    {
        const std::shared_ptr<T>& asset = it->second;

        // AssetStorage 외부에서 아무도 참조하고 있지 않다면
        if (asset.use_count() <= 1)
        {
            assets.erase(it);
        }
    }
}
}
