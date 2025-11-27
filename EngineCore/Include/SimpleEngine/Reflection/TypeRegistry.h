#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Reflection/Meta.h"


namespace se::refl
{
/**
 * 런타임 타입 정보(RTTI)를 중앙에서 관리하는 전역 저장소
 * 컴파일 타임에 수집된 모든 리플렉션 데이터를 런타임에 검색(Look-up)할 수 있게 해줍니다.
 */
class SE_CORE_API TypeRegistry
{
    TypeRegistry() = default;

public:
    ~TypeRegistry() = default;

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    TypeRegistry(TypeRegistry&&) = delete;
    TypeRegistry& operator=(TypeRegistry&&) = delete;

    static TypeRegistry& GetInstance();

public:
    void RegisterType(TypeInfo&& type_info);

    template <typename T>
    [[nodiscard]] Optional<const TypeInfo&> Find() const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const TypeId& type_id) const;
    [[nodiscard]] Optional<const TypeInfo&> Find(const StringName& type_name) const;
    [[nodiscard]] const auto& GetAllTypes() const { return type_map; }

private:
    HashMap<StringName, TypeId> name_map;
    HashMap<TypeId, TypeInfo> type_map;
};

template <typename T>
Optional<const TypeInfo&> TypeRegistry::Find() const
{
    return Find(TypeId::Get<T>());
}
}
