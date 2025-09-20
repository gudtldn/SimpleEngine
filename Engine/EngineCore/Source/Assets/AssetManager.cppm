export module SE.Assets:AssetManager;
import :AssetStorage;
import :Handle;

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
     * @param virtual_path 에셋의 가상 경로 위치, ex) Game://Foo/Bar/MyAsset.png
     * @return 에셋의 Handle
     */
    template <typename T>
    Handle<T> Load(const std::filesystem::path& virtual_path);

    /**
     * Handle로부터 실제 에셋을 가져옵니다.
     * @tparam T 가져오려는 에셋 타입
     * @param handle 실제 에셋을 불러올 Handle
     * @return 에셋의 참조. Handle이 유효하지 않을 경우 nullptr를 반환합니다.
     */
    template <typename T>
    std::shared_ptr<T> Get(Handle<T> handle);

private:
    /**
     * AssetStorage를 새로 만들거나 가져옵니다.
     * @tparam T 가져오려는 Asset의 타입
     * @return AssetStorage<T>의 참조
     */
    template <typename T>
    AssetStorage<T>& GetOrCreateStorage();

private:
    unordered_map<std::type_index, std::unique_ptr<IAssetStorage>> storages;
};

template <typename T>
Handle<T> AssetManager::Load(const std::filesystem::path& virtual_path)
{
    Handle<T> handle = {
        .asset_id = StringName{ virtual_path.generic_u8string() }
    };

    // TODO: 에셋이 로딩중인지 확인
    // TODO: 비동기로 에셋 로딩

    return handle;
}

template <typename T>
std::shared_ptr<T> AssetManager::Get(Handle<T> handle)
{
    if (!handle.IsValid())
    {
        return nullptr;
    }

    AssetStorage<T>& storage = GetOrCreateStorage<T>();
    return storage.GetAsset(handle.asset_id);
}

template <typename T>
AssetStorage<T>& AssetManager::GetOrCreateStorage()
{
    const std::type_index type_idx = std::type_index(typeid(T));
    if (!storages.contains(type_idx))
    {
        storages[type_idx] = std::make_unique<AssetStorage<T>>();
    }
    return static_cast<AssetStorage<T>&>(*storages[type_idx]);
}
}
