module;
#include "tracy/Tracy.hpp"
export module SE.Assets:AssetStorage;
import :AssetEntry;

import SE.Types;
import std;


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
class AssetStorage : public IAssetStorage
{
public:
    using AssetType = T;

public:
    /** Storage에서 Asset을 가져옵니다. */
    std::shared_ptr<T> GetAsset(const StringName& asset_id);

    /** Storage에 Asset을 추가합니다. */
    void InsertAsset(const StringName& asset_id, const std::shared_ptr<T>& asset);

    /** Asset의 참조가 1개 이하일 때, Asset을 Storage에서 제거합니다. */
    virtual void RemoveReference(const StringName& asset_id) override;

private:
    TracySharedLockable(std::shared_mutex, mutex);
    unordered_map<StringName, std::shared_ptr<AssetEntry<T>>> assets;
};

template <typename T>
std::shared_ptr<T> AssetStorage<T>::GetAsset(const StringName& asset_id)
{
    std::shared_lock lock{ mutex };

    if (auto it = assets.find(asset_id); it != assets.end())
    {
        return it->second;
    }
    return nullptr;
}

template <typename T>
void AssetStorage<T>::InsertAsset(const StringName& asset_id, const std::shared_ptr<T>& asset)
{
    std::unique_lock lock{ mutex };
    assets[asset_id] = asset;
}

template <typename T>
void AssetStorage<T>::RemoveReference(const StringName& asset_id)
{
    std::unique_lock lock{ mutex };

    if (auto it = assets.find(asset_id); it != assets.end())
    {
        if (it->second.use_count() <= 1)
        {
            assets.erase(it);
        }
    }
}
}
