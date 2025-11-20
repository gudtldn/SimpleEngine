#pragma once
#include <filesystem>
#include <memory>

#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Asset/ImportSettings/IAssetImportSettings.h"
#include "SimpleEngine/Core/Concurrency/Coroutine.h"
#include "SimpleEngine/Reflection/TypeSignature.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::asset
{
class SE_CORE_API IAssetLoader
{
public:
    virtual ~IAssetLoader() = default;
    virtual concurrency::Task<std::shared_ptr<IAsset>> Load(
        const std::filesystem::path& physical_path,
        const IAssetImportSettings* import_settings
    ) = 0;

protected:
    /**
     * ImportSetting을 안전하게 캐스팅해서 가져옵니다.
     * 설정이 없거나(nullptr), 타입이 맞지 않으면 nullptr를 반환합니다.
     */
    template <typename SettingType>
        requires std::derived_from<SettingType, IAssetImportSettings>
    const SettingType* GetSettings(const IAssetImportSettings* in_settings) const
    {
        if (in_settings)
        {
#if SE_DEBUG_BUILD
            const SettingType* settings = dynamic_cast<const SettingType*>(in_settings);
            SE_ASSERT(settings, "Invalid Asset Import Settings Type. Expected: {}, Actual Type ID mismatch.", refl::GetFullTypeName<SettingType>());
            return settings;
#else
            return static_cast<const SettingType*>(in_settings);
#endif
        }
        return nullptr;
    }
};
}
