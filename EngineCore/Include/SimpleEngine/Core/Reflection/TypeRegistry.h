#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeInfo.h"

#include <mutex>


namespace se
{
/** 리플렉션과 직렬화의 등록 상태를 지키는 전역 락을 가져옵니다. */
[[nodiscard]] SE_CORE_API std::recursive_mutex& RegistrationMutex();

/**
 * EnsureRegistered가 타입 하나를 등록하는 동안 여는 범위
 * 열린 범위의 수를 세며, RegistrationMutex()를 잡은 채로만 만들고 묻습니다.
 */
class SE_CORE_API RegistrationScope
{
public:
    RegistrationScope();
    ~RegistrationScope();

    RegistrationScope(const RegistrationScope&) = delete;
    RegistrationScope& operator=(const RegistrationScope&) = delete;
    RegistrationScope(RegistrationScope&&) = delete;
    RegistrationScope& operator=(RegistrationScope&&) = delete;

public:
    /** 등록 중인 타입이 있는지 확인합니다. 있으면 레지스트리에 아직 다 채워지지 않은 슬롯이 있을 수 있습니다. */
    [[nodiscard]] static bool IsRegistering();
};

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

    /** 지금까지 등록된 모든 TypeInfo의 스냅샷을 반환합니다. */
    [[nodiscard]] Array<const TypeInfo*> GetAllTypes() const;

private:
    /** 각 타입의 TypeInfo 저장소 */
    HashMap<TypeId, TypeInfo> type_map;

    /** 각 구조체/클래스 타입의 부모, 필드 정보 저장소 */
    HashMap<TypeId, StructStorage> struct_storage;

    /** 각 enum 타입의 항목 정보 저장소 */
    HashMap<TypeId, Array<EnumEntry>> enum_entry_storage;
};
} // namespace se
