#include "SimpleEngine/Asset/AssetRegistry_DEPRECATED.h"


namespace se::asset
{
void AssetRegistry_DEPRECATED::AddEntry(AssetEntry_DEPRECATED&& entry)
{
    vpath_map[entry.virtual_path] = entry.guid;
    guid_map[entry.guid] = std::move(entry);
}

void AssetRegistry_DEPRECATED::Clear()
{
    guid_map.Clear();
    vpath_map.Clear();
}

Optional<const AssetEntry_DEPRECATED&> AssetRegistry_DEPRECATED::GetEntry(const Guid& guid) const
{
    return guid_map.Find(guid);
}

Optional<const AssetEntry_DEPRECATED&> AssetRegistry_DEPRECATED::GetEntry(const VPath& vpath) const
{
    return vpath_map.Find(vpath).AndThen([this](const Guid& guid)
    {
        return GetEntry(guid);
    });
}

Optional<const Guid&> AssetRegistry_DEPRECATED::GetGuid(const VPath& vpath) const
{
    return vpath_map.Find(vpath);
}
}
