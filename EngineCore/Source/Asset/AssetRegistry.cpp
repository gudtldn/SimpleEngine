#include "SimpleEngine/Asset/AssetRegistry.h"


namespace se::asset
{
void AssetRegistry::SaveToCache(const std::filesystem::path& cache_path)
{
    (void)cache_path;

    // TODO: guid_map을 직렬화해서 저장
}

bool AssetRegistry::LoadFromCache(const std::filesystem::path& cache_path)
{
    (void)cache_path;

    // TODO: guid_map을 역직렬화해서 불러오기
    return false;
}

void AssetRegistry::AddEntry(AssetEntry&& entry)
{
    vpath_map[entry.virtual_path] = entry.guid;
    guid_map[entry.guid] = std::move(entry);
}

void AssetRegistry::Clear()
{
    guid_map.Clear();
    vpath_map.Clear();
}

Optional<const AssetEntry&> AssetRegistry::GetEntry(const Guid& guid) const
{
    return guid_map.Find(guid);
}

Optional<const AssetEntry&> AssetRegistry::GetEntry(const VPath& vpath) const
{
    return vpath_map.Find(vpath).AndThen([this](const Guid& guid)
    {
        return GetEntry(guid);
    });
}

Optional<const Guid&> AssetRegistry::GetGuid(const VPath& vpath) const
{
    return vpath_map.Find(vpath);
}
}
