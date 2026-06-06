#include "SimpleEngine/ECS/ECSRegistry.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"


namespace se
{
ECSRegistry& ECSRegistry::Get()
{
    static ECSRegistry instance;
    return instance;
}

Optional<const ComponentOps&> ECSRegistry::GetComponentOps(const TypeId& type_id) const
{
    return component_operators.Find(type_id);
}

Optional<const ResourceOps&> ECSRegistry::GetResourceOps(const TypeId& type_id) const
{
    return resource_operators.Find(type_id);
}

// TODO: 한계점
// 1. 기본 생성자(default-constructed) 값만 삽입 -- 커스텀 초기화가 필요한 Transient Resource는 별도 처리 필요
// 2. ECSRegistry에 등록된 Resource만 대상 -- 미등록 Resource는 누락됨
void ECSRegistry::InsertDefaultTransientResources(World& world) const
{
    const auto& registry = TypeRegistry::Get();
    for (const auto& [type_id, ops] : resource_operators)
    {
        if (auto info_opt = registry.Find(type_id);
            info_opt.HasValue() && info_opt->flags.IsSet(ETypeFlags::Transient) && !ops.has_resource(world))
        {
            ops.insert_default(world);
        }
    }
}
} // namespace se
