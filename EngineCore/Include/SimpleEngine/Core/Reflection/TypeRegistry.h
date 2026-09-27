#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"

#include <mutex>


namespace se
{
/**
 * 리플렉션과 직렬화의 등록 상태(TypeRegistry, ValueOpsRegistry, TypeRecordRegistry, SerializeOpsRegistry, SerializePlan 저장소)를 지키는 전역 락을 가져옵니다.
 * 이 상태를 읽거나 쓰는 모든 곳이 잡으며, 등록 중 같은 스레드의 재진입을 허용하는 재귀 락입니다.
 * 모든 모듈이 같은 락을 쓰도록 EngineCore의 비템플릿 함수 안에 둡니다.
 */
[[nodiscard]] SE_CORE_API std::recursive_mutex& RegistrationMutex();

/**
 * 구조체/클래스 타입 하나의 부모/필드 목록 저장소
 */
struct StructStorage
{
    Array<BaseInfo> bases;
    Array<FieldInfo> fields;
};

/**
 * 모든 TypeInfo를 소유하는 전역 레지스트리
 * @note 모든 멤버 함수가 RegistrationMutex()를 잡습니다. Emplace 계열이 돌려준 참조는 EnsureRegistered가 락을 잡은 채로 채웁니다.
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

    /** 부모·필드 목록을 저장할 StructStorage를 가져옵니다. */
    [[nodiscard]] StructStorage& EmplaceStructStorage(TypeId id);

    /** enum 항목 목록을 저장할 배열을 가져옵니다. */
    [[nodiscard]] Array<EnumEntry>& EmplaceEnumEntryStorage(TypeId id);

    /** TypeId로 TypeInfo를 찾습니다. (등록되지 않았다면 NullOpt)*/
    [[nodiscard]] Optional<const TypeInfo&> Find(TypeId id) const;

    /**
     * TypeId로 TypeInfo를 찾습니다.
     * @warning 등록되지 않은 타입이면 Assert
     */
    [[nodiscard]] const TypeInfo& FindChecked(TypeId id) const;

    /** 지금까지 등록된 모든 TypeInfo의 스냅샷을 반환합니다. 이후 다른 스레드가 등록한 타입은 담기지 않습니다. */
    [[nodiscard]] Array<const TypeInfo*> GetAllTypes() const;

private:
    /** 각 타입의 TypeInfo 저장소 */
    HashMap<TypeId, TypeInfo> type_map;

    /** 각 구조체/클래스 타입의 부모·필드 정보 저장소 */
    HashMap<TypeId, StructStorage> struct_storage;

    /** 각 enum 타입의 항목 정보 저장소 */
    HashMap<TypeId, Array<EnumEntry>> enum_entry_storage;
};
} // namespace se
