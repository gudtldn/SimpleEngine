#include "SimpleEngine/Core/Serialization/AutoSerialize.h"

#include "SimpleEngine/Core/Container/HashSet.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/Archive.h"


namespace se
{
namespace
{
void AutoSerializeImpl(Archive& ar, const TypeInfo_v1& info, void* instance, HashSet<void*>& visited) // NOLINT(*-no-recursion)
{
    if (!instance)
    {
        return;
    }

    // 부모 타입의 프로퍼티를 먼저 직렬화 (다중 상속 포함, 주소 기준 dedup)
    for (const BaseInfo_v1& base : info.bases)
    {
        if (const auto parent = TypeRegistry_v1::Get().Find(base.base_id))
        {
            void* base_instance = base.upcast(instance);
            if (visited.Insert(base_instance))
            {
                AutoSerializeImpl(ar, *parent, base_instance, visited);
            }
        }
    }

    // 현재 타입의 프로퍼티 순회
    for (const PropertyInfo_v1& prop : info.properties)
    {
        // Transient 프로퍼티는 건너뜀
        if (prop.metadata.flags.IsAnySet(EPropertyFlags_v1::Transient))
        {
            continue;
        }

        // serialize 콜백이 없으면 건너뜀
        if (!prop.serialize)
        {
            continue;
        }

        // HintNextName으로 프로퍼티 이름 설정 후 직렬화
        ar(prop.name);
        prop.serialize(ar, prop.accessor.get_mut(instance));
    }
}
} // namespace

void AutoSerialize(Archive& ar, const TypeInfo_v1& info, void* instance)
{
    HashSet<void*> visited;
    AutoSerializeImpl(ar, info, instance, visited);
}

void AutoSerialize(Archive& ar, const TypeId_v1& type_id, void* instance)
{
    const TypeInfo_v1& info = TypeRegistry_v1::Get().FindChecked(type_id);
    AutoSerialize(ar, info, instance);
}
} // namespace se
