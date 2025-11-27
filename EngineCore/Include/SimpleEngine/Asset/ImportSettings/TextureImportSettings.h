#pragma once
#include "SimpleEngine/Asset/ImportSettings/IAssetImportSettings.h"


namespace se::asset
{
/**
 * 텍스처 리소스용 ImportSettings
 */
class SE_CORE_API TextureImportSettings : public AssetImportSettingsBase<TextureImportSettings>
{
public:
    // TODO: 나중에 .png 파일을 임포트할 때 파일 이름에 _N, _ORM 등이 포함되어 있으면,
    // 자동으로 is_srgb = false로 설정해 주는 로직을 EditorAssetSubsystem::ImportAsset 쪽에 추가
    bool is_srgb = true;       // Color(true) vs Data(false)
    bool generate_mips = true; // 밉맵 생성 여부

public:
    virtual void Serialize(core::Archive& ar) override
    {
        ar("is_srgb") << is_srgb;
        ar("generate_mips") << generate_mips;
    }
};
}
