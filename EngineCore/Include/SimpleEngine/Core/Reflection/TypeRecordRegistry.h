#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/TypeRecord.h"


namespace se
{
/**
 * 모든 TypeRecord(캐스트 테이블)를 소유하는 전역 레지스트리
 */
class SE_CORE_API TypeRecordRegistry
{
    TypeRecordRegistry() = default;

public:
    /** 전역 싱글톤 인스턴스를 가져옵니다. */
    [[nodiscard]] static TypeRecordRegistry& Get();

    /** id의 캐스트 테이블을 만들어 설치합니다. 부모들이 먼저 등록되어 있어야 합니다. */
    void Install(TypeId id);

    /** TypeId로 TypeRecord를 찾습니다. (등록되지 않았다면 NullOpt) */
    [[nodiscard]] Optional<const TypeRecord&> Find(TypeId id) const;

private:
    HashMap<TypeId, TypeRecord> record_map;
};
} // namespace se
