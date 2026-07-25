#pragma once

#include "SimpleEngine/Asset/AssetRegistry.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/HashSet.h"
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

    /** 이번 임포트 실행에서 이미 사용된 (고유화된) 이름 집합. 이름 dedup의 단일 지점 */
    HashSet<String> used_names_this_run;

    /**
     * 서브 에셋의 이름을 고유하게 만들고, GUID를 발급하거나 기존 값을 반환합니다.
     * @details desired_name이 이번 실행에서 이미 사용되었다면 "_1", "_2" ... 접미사로 고유하게 만든 뒤,
     *          고유한 이름으로 reserved_sub_guids를 조회하여 GUID를 재사용하거나 새로 발급합니다.
     * @todo M3(stable_key 도입)에서 이 함수는 AllocateSubAsset(stable_key, desired_name)으로 시그니처가 확장될 예정입니다.
     * @return 고유한 이름과 GUID
     */
    [[nodiscard]] std::pair<String, Guid> AllocateSubAsset(const String& desired_name)
    {
        String unique_name = desired_name;
        u32 suffix = 1;
        while (used_names_this_run.Contains(unique_name))
        {
            unique_name = String::Format("{}_{}", desired_name, suffix);
            ++suffix;
        }
        used_names_this_run.Insert(unique_name);

        if (const auto existing = reserved_sub_guids.Find(unique_name))
        {
            return { std::move(unique_name), *existing };
        }

        const Guid new_guid = Guid::NewGuid();
        out_allocated_sub_guids.Push({ unique_name, new_guid });
        return { std::move(unique_name), new_guid };
    }
};
} // namespace se::editor
