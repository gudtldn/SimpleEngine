#pragma once
#include <filesystem>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::utility
{
class PathResolver;
}

namespace se::asset
{
/**
 * 런타임에 필요한 Asset의 메타데이터
 */
struct AssetEntry
{
    // Asset의 고유 식별자
    Guid guid;

    // Asset의 TypeId
    refl::TypeId asset_type;

    // "Asset://"으로 시작하는 에셋의 가상 경로
    VPath virtual_path;

    // 이 에셋이 의존하는 다른 에셋 GUID 목록
    Array<Guid> dependencies;

    // TODO: Archive를 사용한 직렬화/역직렬화 함수 추가
};


/**
 * 프로젝트 내의 모든 에셋의 메타데이터를 관리하는 중앙 데이터베이스
 */
class SE_CORE_API AssetRegistry
{
public:
    /** 런타임 캐시 파일(.cache/AssetRegistry.cache)을 로드합니다. */
    void SaveToCache(const std::filesystem::path& cache_path);

    /** 에디터가 빌드한 캐시를 저장합니다. */
    bool LoadFromCache(const std::filesystem::path& cache_path);

    [[nodiscard]] Optional<const AssetEntry&> GetEntry(const Guid& guid) const;
    [[nodiscard]] Optional<const AssetEntry&> GetEntry(const VPath& vpath) const;
    [[nodiscard]] Optional<const Guid&> GetGuid(const VPath& vpath) const;

    // Editor에서 Entry를 채우기 위한 API
    void AddEntry(AssetEntry&& entry);
    void Clear();

private:
    HashMap<Guid, AssetEntry> guid_map;
    HashMap<VPath, Guid> vpath_map;
};
}
