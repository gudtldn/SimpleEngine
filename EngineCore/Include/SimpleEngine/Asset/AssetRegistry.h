#pragma once
#include <filesystem>

#include "SimpleEngine/Asset/ImportSettings_DEPRECATED/IAssetImportSettings.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Reflection/TypeId.h"
#include "SimpleEngine/Utility/Debug.h"


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

    // Loader의 TypeId
    refl::TypeId loader_type;

    // "Asset://"으로 시작하는 에셋의 가상 경로
    VPath virtual_path;

    // 이 에셋이 의존하는 다른 에셋 GUID 목록
    Array<Guid> dependencies;

    // 에셋을 불러올 때 사용할 Settings
    std::shared_ptr<IAssetImportSettings> import_settings = nullptr;

public:
    template <typename SettingType>
        requires std::derived_from<SettingType, IAssetImportSettings>
    const SettingType* GetSettings() const
    {
        if (import_settings)
        {
#if SE_DEBUG_BUILD
            const SettingType* settings = dynamic_cast<const SettingType*>(import_settings.get());
            SE_ASSERT(settings, "Invalid Asset Import Settings Type. Expected: {}, Actual Type ID mismatch.", refl::GetFullTypeName<SettingType>());
            return settings;
#else
            return static_cast<const SettingType*>(import_settings.get());
#endif
        }
        return nullptr;
    }

    friend void Serialize(core::Archive& ar, AssetEntry& entry)
    {
        String virtual_path_str;
        if (ar.IsSaving())
        {
            virtual_path_str = entry.virtual_path.ToString();
        }

        // TODO: 저장할 때 버전정보에 따른 로직 추가

        ar("guid") << entry.guid;
        ar("asset_type") << entry.asset_type;
        ar("loader_type") << entry.loader_type;
        ar("virtual_path") << virtual_path_str;
        ar("dependencies") << entry.dependencies;

        if (entry.import_settings)
        {
            ar("import_settings") << *entry.import_settings;
        }

        if (ar.IsLoading())
        {
            entry.virtual_path = VPath{ virtual_path_str };
        }
    }
};


/**
 * 프로젝트 내의 모든 에셋의 메타데이터를 관리하는 중앙 데이터베이스
 */
class SE_CORE_API AssetRegistry
{
public:
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
