#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Reflection/ValueOps.h"


namespace se
{
/**
 * 모든 ValueOps를 소유하는 전역 레지스트리
 */
class SE_CORE_API ValueOpsRegistry
{
    ValueOpsRegistry() = default;

public:
    /** 전역 싱글톤 인스턴스를 가져옵니다. */
    [[nodiscard]] static ValueOpsRegistry& Get();

    /** 주어진 타입의 ValueOps를 등록합니다. 이미 있으면 덮어씁니다. */
    void Install(TypeId id, const ValueOps& ops);

    /** TypeId로 ValueOps를 찾습니다. (등록되지 않았다면 NullOpt) */
    [[nodiscard]] Optional<const ValueOps&> Find(TypeId id) const;

private:
    HashMap<TypeId, ValueOps> ops_map;
};
} // namespace se
