#include "SimpleEngine/Core/Reflection/TypeRecordRegistry.h"

#include "SimpleEngine/Core/Reflection/TypeRegistry.h"

#include <utility>


namespace se
{
namespace
{
/** id의 서브오브젝트를 accumulated_offset 기준으로 평탄화해 out에 담습니다. */
void Flatten(TypeId id, usize accumulated_offset, Array<CastEntry>& out) // NOLINT(*-no-recursion)
{
    out.Push({ .type = id, .offset = accumulated_offset });

    const auto info = TypeRegistry::Get().Find(id);
    if (!info.HasValue())
    {
        return; // 등록되지 않은 부모가 있다면 건너뜀
    }

    const auto struct_info = info->AsStruct();
    if (!struct_info.HasValue())
    {
        return;
    }

    for (const BaseInfo& base : struct_info->bases)
    {
        Flatten(base.type, accumulated_offset + base.offset, out);
    }
}
} // namespace

TypeRecordRegistry& TypeRecordRegistry::Get()
{
    static TypeRecordRegistry instance;
    return instance;
}

void TypeRecordRegistry::Install(TypeId id)
{
    if (record_map.Contains(id))
    {
        return;
    }

    TypeRecord record{ .id = id, .all_bases = {} };
    Flatten(id, 0, record.all_bases);
    record_map.Entry(id).OrDefault() = std::move(record);
}

Optional<const TypeRecord&> TypeRecordRegistry::Find(TypeId id) const
{
    return record_map.Find(id);
}
} // namespace se
