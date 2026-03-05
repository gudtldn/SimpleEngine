#include "SimpleEngine/Asset/AssetPool.h"


namespace se::asset
{
std::shared_ptr<AssetSlot> AssetPool::Find(const AssetId& id) const
{
    std::shared_lock read_lock(slot_mutex);
    if (const Optional slot_opt = slots.Find(id))
    {
        return *slot_opt;
    }
    return nullptr;
}

std::shared_ptr<AssetSlot> AssetPool::FindOrCreate(const AssetId& id, const TypeId& type_id, const AssetPath& asset_path)
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
        return std::make_shared<AssetSlot>(id, type_id, asset_path);
    });
}

void AssetPool::Remove(const AssetId& id)
{
    std::unique_lock write_lock(slot_mutex);
    slots.Remove(id);
}

uint32 AssetPool::CollectGarbage()
{
    using SlotKey = decltype(slots)::KeyType;
    using SlotValue = decltype(slots)::ValueType;

    std::unique_lock write_lock(slot_mutex);
    const usize remove_count = slots.RemoveIf([](const SlotKey&, const SlotValue& slot_ptr)
    {
        // 사용되고 있는 외부 Handle이 없는 경우 제거
        return slot_ptr.use_count() == 1;
    });
    return static_cast<uint32>(remove_count);
}

uint32 AssetPool::GetCount() const
{
    std::shared_lock read_lock(slot_mutex);
    return static_cast<uint32>(slots.Len());
}
}  // namespace se::asset
