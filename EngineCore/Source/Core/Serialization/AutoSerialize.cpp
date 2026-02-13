#include "Core/Serialization/AutoSerialize.h"

#include "Core/Reflection/TypeRegistry.h"
#include "Core/Serialization/Archive.h"


namespace se
{
void AutoSerialize(Archive& ar, const TypeInfo& info, void* instance)
{
    // 부모 타입의 프로퍼티를 먼저 직렬화 (상속 체인 재귀)
    if (info.base_or_inner_id.IsValid() && info.kind == ETypeKind::Struct)
    {
        if (const auto parent_opt = TypeRegistry::Get().Find(info.base_or_inner_id))
        {
            AutoSerialize(ar, parent_opt.Value(), instance);
        }
    }

    // 현재 타입의 프로퍼티 순회
    for (const PropertyInfo& prop : info.properties)
    {
        // Transient 프로퍼티는 건너뜀
        if (prop.metadata.flags.IsAnySet(EPropertyFlags::Transient))
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
        prop.serialize(ar, prop.accessor.get_ptr(instance));
    }
}

void AutoSerialize(Archive& ar, const TypeId& type_id, void* instance)
{
    const TypeInfo& info = TypeRegistry::Get().FindChecked(type_id);
    AutoSerialize(ar, info, instance);
}
}  // namespace se
