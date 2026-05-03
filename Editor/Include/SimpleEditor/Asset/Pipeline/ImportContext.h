#pragma once

#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Guid.h"

#include <utility>


namespace se::editor
{
/**
 * 서브 에셋(Sub-asset) GUID 발급 및 관리를 중재하는 컨텍스트
 */
struct ImportContext
{
    /** 기존 .meta에 기록된 이름-GUID 맵 (reimport 시 유지용) */
    const HashMap<String, Guid>& reserved_sub_guids;

    /** 에셋 참조 해소용 레지스트리 */
    const AssetRegistry& registry;

    /** 신규 발급된 이름-GUID 목록 (이후 .meta에 병합됨) */
    Array<std::pair<String, Guid>>& out_allocated_sub_guids;

    /**
     * 서브 에셋의 GUID를 발급하거나 기존 값을 반환합니다.
     * @details 예약된 GUID가 있다면 재사용하고, 없다면 새로 생성하여 out_guid에 추가합니다.
     */
    // ReSharper disable once CppMemberFunctionMayBeConst
    [[nodiscard]] Guid AllocateSubAssetGuid(const String& sub_asset_name)
    {
        if (const auto existing = reserved_sub_guids.Find(sub_asset_name))
        {
            return *existing;
        }

        const Guid new_guid = Guid::NewGuid();
        out_allocated_sub_guids.Push({ sub_asset_name, new_guid });
        return new_guid;
    }
};
} // namespace se::editor
