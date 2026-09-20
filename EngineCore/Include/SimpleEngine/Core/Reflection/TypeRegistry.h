#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"


namespace se
{
/**
 * 모든 TypeInfo를 소유하는 전역 레지스트리
 */
class SE_CORE_API TypeRegistry
{
    TypeRegistry() = default;

public:
    /** 전역 싱글톤 인스턴스를 가져옵니다. */
    [[nodiscard]] static TypeRegistry& Get();

    /**
     * 주어진 TypeId에 대한 TypeInfo 슬롯을 가져옵니다.
     * 이미 있으면 기존 슬롯을, 없으면 새로 만든 빈 슬롯을 반환합니다.
     */
    [[nodiscard]] TypeInfo& Emplace(TypeId id);

    /** 부모 목록을 저장할 배열을 가져옵니다. */
    [[nodiscard]] Array<BaseInfo>& EmplaceBaseStorage(TypeId id);

    /** 필드 목록을 저장할 배열을 가져옵니다. */
    [[nodiscard]] Array<FieldInfo>& EmplaceFieldStorage(TypeId id);

    /** enum 항목 목록을 저장할 배열을 가져옵니다. */
    [[nodiscard]] Array<EnumEntry>& EmplaceEnumEntryStorage(TypeId id);

    /** TypeId로 TypeInfo를 찾습니다. (등록되지 않았다면 NullOpt)*/
    [[nodiscard]] Optional<const TypeInfo&> Find(TypeId id) const;

    /**
     * TypeId로 TypeInfo를 찾습니다.
     * @warning 등록되지 않은 타입이면 Assert
     */
    [[nodiscard]] const TypeInfo& FindChecked(TypeId id) const;

    /** 지금까지 등록된 모든 TypeInfo를 lazy-view로 반환합니다. */
    [[nodiscard]] auto GetAllTypes() const
    {
        return type_map.Iter().Map([](const auto& pair) -> const TypeInfo*
        {
            return &pair.second;
        });
    }

private:
    /** 각 타입의 TypeInfo 저장소 */
    HashMap<TypeId, TypeInfo> type_map;

    /** 각 타입의 부모 정보 저장소 */
    HashMap<TypeId, Array<BaseInfo>> base_storage;

    /** 각 타입의 필드 정보 저장소 */
    HashMap<TypeId, Array<FieldInfo>> field_storage;

    /** 각 enum 타입의 항목 정보 저장소 */
    HashMap<TypeId, Array<EnumEntry>> enum_entry_storage;
};
} // namespace se
