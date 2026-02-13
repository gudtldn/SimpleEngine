#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"


namespace se::detail
{
bool IsTypeDerivedFrom(const TypeId& derived_id, const TypeId& base_id)
{
    TypeId current = derived_id;
    while (current.IsValid())
    {
        if (current == base_id)
        {
            return true;
        }

        const Optional info_opt = TypeRegistry::Get().Find(current);
        if (!info_opt.HasValue())
        {
            break;
        }

        // 부모 클래스로 이동하여 계속 검색
        current = info_opt->base_or_inner_id;
    }
    return false;
}

bool IsTypeImplementsInterface(const TypeId& type_id, const TypeId& interface_id)
{
    TypeId current_id = type_id;
    while (current_id.IsValid())
    {
        const Optional info_opt = TypeRegistry::Get().Find(current_id);
        if (!info_opt.HasValue())
        {
            break;
        }

        // 현재 클래스가 인터페이스를 가지고 있는지 확인
        if (info_opt->interfaces.Contains(interface_id))
        {
            return true;
        }

        // 부모 클래스로 이동하여 계속 검색
        current_id = info_opt->base_or_inner_id;
    }
    return false;
}

void* CastToInterface(void* instance, const TypeId& type_id, const TypeId& interface_id)
{
    if (!instance)
    {
        return nullptr;
    }

    TypeId current_id = type_id;
    while (current_id.IsValid())
    {
        const Optional info_opt = TypeRegistry::Get().Find(current_id);
        if (!info_opt.HasValue())
        {
            break;
        }

        if (const Optional interface_opt = info_opt->interfaces.Find(interface_id))
        {
            return interface_opt->caster(instance);
        }

        // 부모 클래스로 이동
        current_id = info_opt->base_or_inner_id;
    }

    return nullptr;
}
} // namespace se::detail
