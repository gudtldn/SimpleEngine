#include "SimpleEditor/Asset/Pipeline/ImportResult.h"


namespace se::editor
{
ImportResult::ImportResult(
    Array<std::shared_ptr<asset::AssetBase>> assets,
    HashMap<String, uint32> name_to_index,
    Array<Guid> stable_guids,
    uint32 main_asset_index
)
    : assets(std::move(assets))
    , name_to_index(std::move(name_to_index))
    , stable_guids(std::move(stable_guids))
    , main_asset_index(main_asset_index)
{
}

std::shared_ptr<asset::AssetBase> ImportResult::GetMainAsset() const
{
    if (main_asset_index < assets.Len())
    {
        return assets[main_asset_index];
    }
    return nullptr;
}

std::shared_ptr<asset::AssetBase> ImportResult::GetAsset(uint32 index) const
{
    if (index < assets.Len())
    {
        return assets[index];
    }
    return nullptr;
}

std::shared_ptr<asset::AssetBase> ImportResult::FindByName(StringView name) const
{
    if (const auto index_opt = name_to_index.Find(name))
    {
        if (*index_opt < assets.Len())
        {
            return assets[*index_opt];
        }
    }
    return nullptr;
}

Array<StringView> ImportResult::GetAllNames() const
{
    return name_to_index.Keys<StringView>();
}


uint32 ImportResult::Builder::RegisterAsset(std::shared_ptr<asset::AssetBase> asset, const String& name, const Guid& stable_guid)
{
    const uint32 index = static_cast<uint32>(assets.Len());
    assets.Push(std::move(asset));

    if (!name.IsEmpty())
    {
        String unique_name = MakeUniqueName(name);
        name_to_index.Insert(std::move(unique_name), index);
    }

    if (stable_guid.IsValid())
    {
        index_to_stable_guid.Insert(index, stable_guid);
    }

    return index;
}

void ImportResult::Builder::SetMainAssetIndex(uint32 index)
{
    main_asset_index = index;
}

ImportResult ImportResult::Builder::Build()
{
    // assets 배열 크기에 맞춰 stable_guids 배열 생성
    Array<Guid> stable_guids_array;
    stable_guids_array.Resize(assets.Len());
    for (const auto& [index, guid] : index_to_stable_guid)
    {
        if (index < stable_guids_array.Len())
        {
            stable_guids_array[index] = guid;
        }
    }

    return {
        std::move(assets),
        std::move(name_to_index),
        std::move(stable_guids_array),
        main_asset_index
    };
}

String ImportResult::Builder::MakeUniqueName(const String& base_name)
{
    // 이름이 이미 존재하지 않으면 그대로 반환
    if (!name_to_index.Contains(base_name))
    {
        return base_name;
    }

    // 중복 시 suffix 추가: Name_1, Name_2, ...
    String copy_name = base_name;
    uint32& next_suffix = next_suffix_map.Entry(copy_name).OrInsert(1);
    do
    {
        copy_name = String::Format("{}_{}", base_name, next_suffix++);
    }
    while (name_to_index.Contains(copy_name));

    return copy_name;
}
} // namespace se::editor
