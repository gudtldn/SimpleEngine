#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"


namespace se::detail
{
bool IsTypeDerivedFrom(const TypeId& derived_id, const TypeId& base_id)
{
    if (derived_id == base_id)
    {
        return true;
    }

    const auto info_opt = TypeRegistry::Get().Find(derived_id);
    if (!info_opt.HasValue())
    {
        return false;
    }

    return std::ranges::any_of(info_opt->bases, [&](const BaseInfo& base)
    {
        return IsTypeDerivedFrom(base.base_id, base_id);
    });
}

void* TryUpcast(void* instance, const TypeId& from, const TypeId& to)
{
    // 현재 타입이 목표 타입과 일치하면 즉시 반환
    if (from == to)
    {
        return instance;
    }

    const auto info_opt = TypeRegistry::Get().Find(from);
    if (!info_opt.HasValue())
    {
        return nullptr;
    }

    // bases 배열을 순회하며 재귀적으로 탐색 (인터페이스도 bases에 포함)
    for (const BaseInfo& base : info_opt->bases)
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
