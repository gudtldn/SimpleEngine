#include "SimpleEngine/Asset/AssetPool.h"


namespace se::asset
{
Optional<HandleData> AssetPool::Find(const AssetId& id) const
{
    return table.Find(id);
}

HandleData AssetPool::FindOrCreate(const AssetId& id, const TypeId& type_id, const AssetPath& asset_path)
{
    return table.FindOrCreate(id, type_id, asset_path);
}

void AssetPool::Remove(const AssetId& id)
{
    if (const Optional<HandleData> handle_data = table.Find(id))
    {
        table.EvictSlot(handle_data->index);
    }
}

uint32 AssetPool::CollectGarbage()
{
    return table.CollectGarbage();
}

uint32 AssetPool::GetCount() const
{
    return table.GetCount();
}
} // namespace se::asset
