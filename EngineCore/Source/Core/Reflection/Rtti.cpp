#include "SimpleEngine/Core/Reflection/Rtti.h"
#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"
#include "SimpleEngine/Utility/Debug.h"

#include <algorithm>


namespace se
{
namespace
{
struct CastLookup
{
    usize from_offset = 0;
    usize to_offset = 0;
    usize from_count = 0;
    usize to_count = 0;
};

[[nodiscard]] CastLookup LookupCast(const TypeRecord& record, TypeId from, TypeId to) noexcept
{
    CastLookup lookup;
    for (const CastEntry& entry : record.all_bases)
    {
        if (entry.type == from)
        {
            lookup.from_offset = entry.offset;
            ++lookup.from_count;
        }
        if (entry.type == to)
        {
            lookup.to_offset = entry.offset;
            ++lookup.to_count;
        }
    }
    return lookup;
}
} // namespace

void* CastById(void* instance, TypeId from_id, TypeId to_id, const TypeRecord& dynamic_record) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }

    if (from_id == to_id)
    {
        return instance;
    }

    const CastLookup lookup = LookupCast(dynamic_record, from_id, to_id);

    SE_ASSERT(lookup.from_count != 0, "CastById: static type missing from dynamic bases. Did you forget SE_BASE?");

    if (lookup.from_count != 1 || lookup.to_count != 1)
    {
        return nullptr;
    }

    u8* complete = static_cast<u8*>(instance) - lookup.from_offset;
    return complete + lookup.to_offset;
}

void* CompleteObjectOfById(void* instance, TypeId from_id, const TypeRecord& dynamic_record) noexcept
{
    if (instance == nullptr)
    {
        return nullptr;
    }

    const CastLookup lookup = LookupCast(dynamic_record, from_id, from_id);
    if (lookup.from_count != 1)
    {
        return nullptr;
    }

    return static_cast<u8*>(instance) - lookup.from_offset;
}

bool IsAById(TypeId dynamic_id, TypeId target_id) noexcept
{
    if (dynamic_id == target_id)
    {
        return true;
    }

    const auto record = TypeRecordRegistry::Get().Find(dynamic_id);
    if (!record.HasValue())
    {
        return false;
    }

    return std::ranges::any_of(record->all_bases, [target_id](const CastEntry& entry)
    {
        return entry.type == target_id;
    });
}
} // namespace se
