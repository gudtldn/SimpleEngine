#include "Asset/AssetCache.h"


namespace se::asset
{
std::shared_ptr<AssetSlot> AssetCache::Find(const AssetId& id) const
{
    std::shared_lock read_lock(slot_mutex);
    if (const Optional slot_opt = slots.Find(id))
    {
        return *slot_opt;
    }
    return nullptr;
}

std::shared_ptr<AssetSlot> AssetCache::FindOrCreate(const AssetId& id, const TypeId& type_id, const Path& path)
{
    {
        std::shared_lock read_lock(slot_mutex);
        if (const Optional slot_opt = slots.Find(id))
        {
            return *slot_opt;
        }
    }

    std::unique_lock write_lock(slot_mutex);
    return slots.Entry(id).OrInsertWith([&]
    {
        return std::make_shared<AssetSlot>(id, type_id, path);
    });
}

void AssetCache::Remove(const AssetId& id)
{
    std::unique_lock write_lock(slot_mutex);
    slots.Remove(id);
}

uint32 AssetCache::CollectGarbage()
{
    using SlotKey = decltype(slots)::KeyType;
    using SlotValue = decltype(slots)::ValueType;

    std::unique_lock write_lock(slot_mutex);
    return slots.RemoveIf([](const SlotKey&, const SlotValue& slot_ptr)
    {
        // 사용되고 있는 외부 Handle이 없는 경우 제거
        return slot_ptr.use_count() == 1;
    });
}

uint32 AssetCache::GetCount() const
{
    std::shared_lock read_lock(slot_mutex);
    return static_cast<uint32>(slots.Len());
}
}  // namespace se::asset
