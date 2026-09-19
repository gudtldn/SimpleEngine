#include "../../../../Include/SimpleEngine/Core/Reflection/Legacy/Cast.h"
#include "../../../../Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"


namespace se::detail
{
bool IsTypeDerivedFrom(const TypeId_v1& derived_id, const TypeId_v1& base_id)
{
    if (derived_id == base_id)
    {
        return true;
    }

    const auto info_opt = TypeRegistry_v1::Get().Find(derived_id);
    if (!info_opt.HasValue())
    {
        return false;
    }

    return std::ranges::any_of(info_opt->bases, [&](const BaseInfo_v1& base)
    {
        return IsTypeDerivedFrom(base.base_id, base_id);
    });
}

void* TryUpcast(void* instance, const TypeId_v1& from, const TypeId_v1& to)
{
    if (!instance)
    {
        return nullptr;
    }

    // 현재 타입이 목표 타입과 일치하면 즉시 반환
    if (from == to)
    {
        return instance;
    }

    const auto info_opt = TypeRegistry_v1::Get().Find(from);
    if (!info_opt.HasValue())
    {
        return nullptr;
    }

    // bases 배열을 순회하며 재귀적으로 탐색 (인터페이스도 bases에 포함)
    for (const BaseInfo_v1& base : info_opt->bases)
    {
        void* adj = base.upcast(instance);
        if (void* result = TryUpcast(adj, base.base_id, to))
        {
            return result;
        }
    }

    return nullptr;
}
} // namespace se::detail
