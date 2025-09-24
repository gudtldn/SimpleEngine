module;
#include "tracy/Tracy.hpp"
export module SE.Assets:AssetManager;
import :AssetStorage;
import :AssetHandle;

import SE.Types;
import std;


export namespace se::assets
{
/**
 *
 */
class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;

public:
    /**
     * 에셋을 비동기로 가져옵니다.
     * @tparam T 가져오려는 에셋 타입
     * @param virtual_path 에셋의 가상 경로 위치, ex) Assets://Foo/Bar/MyAsset.png
     * @return 에셋의 Handle
     */
    template <typename T>
        requires loaders::AssetLoadable<T>
    AssetHandle<T> Load(const std::filesystem::path& virtual_path);

    /**
     * 에셋을 동기 형태로 가져옵니다. (에셋을 불러오는 동안 Thread가 Blocking됩니다!)
     * @tparam T 가져오려는 에셋 타입
     * @param virtual_path 에셋의 가상 경로 위치, ex) Assets://Foo/Bar/MyAsset.png
     * @return 불러온 에셋
     */
    template <typename T>
        requires loaders::AssetLoadable<T>
    std::shared_ptr<T> LoadSynchronous(const std::filesystem::path& virtual_path);

    /**
     * Handle로부터 실제 에셋을 가져옵니다. 만약 에셋이 아직 로딩 중이라면, 로딩이 끝날 때까지 기다립니다.
     * @tparam T 가져오려는 에셋 타입
     * @param handle 실제 에셋을 불러올 Handle
     * @return 에셋의 참조. Handle이 유효하지 않을 경우 nullptr를 반환합니다.
     */
    template <typename T>
    std::shared_ptr<T> Get(AssetHandle<T> handle);

private:
    /**
     * AssetStorage를 새로 만들거나 가져옵니다.
     * @tparam T 가져오려는 Asset의 타입
     * @return AssetStorage<T>의 참조
     */
    template <typename T>
    AssetStorage<T>& GetOrCreateStorage();

private:
    unordered_map<std::type_index, std::shared_ptr<IAssetStorage>> storages;
};

template <typename T>
    requires loaders::AssetLoadable<T>
AssetHandle<T> AssetManager::Load(const std::filesystem::path& virtual_path)
{
    AssetHandle<T> handle = {
        .asset_id = StringName{ virtual_path.generic_u8string() }
    };

    // TODO: 에셋이 로딩중인지 확인
    // TODO: 비동기로 에셋 로딩
    GetOrCreateStorage<T>().LoadAssetAsync(handle.asset_id, virtual_path);

    return handle;
}

template <typename T> requires loaders::AssetLoadable<T>
std::shared_ptr<T> AssetManager::LoadSynchronous(const std::filesystem::path& virtual_path)
{
    return Get<T>(Load<T>(virtual_path));
}

template <typename T>
std::shared_ptr<T> AssetManager::Get(AssetHandle<T> handle)
{
    if (!handle.IsValid())
    {
        return nullptr;
    }

    AssetStorage<T>& storage = GetOrCreateStorage<T>();
    return storage.GetAssetOrWait(handle.asset_id);
}

template <typename T>
AssetStorage<T>& AssetManager::GetOrCreateStorage()
{
    const std::type_index type_idx = std::type_index(typeid(T));
    if (!storages.contains(type_idx))
    {
        storages[type_idx] = std::make_shared<AssetStorage<T>>();
    }
    return *std::static_pointer_cast<AssetStorage<T>>(storages[type_idx]);
}
}
