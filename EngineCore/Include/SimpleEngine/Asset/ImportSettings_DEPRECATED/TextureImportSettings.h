#pragma once
#include "SimpleEngine/Asset/ImportSettings_DEPRECATED/IAssetImportSettings.h"


namespace se::asset
{
/** 텍스처 압축 포맷 */
enum class ETextureCompression : uint8
{
    None = 0, // 압축 안 함 (RGBA8)
    BC1,      // DXT1 (RGB, 1-bit Alpha)
    BC3,      // DXT5 (RGBA, Interpolated Alpha)
    BC4,      // Grayscale (R channel only)
    BC5,      // Normal Map (RG channels)
    BC7,      // High Quality RGBA
};

/** 필터링 모드 */
enum class ETextureFilter : uint8
{
    Nearest, // 도트(픽셀) 느낌
    Linear,  // 부드럽게
};

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

    ETextureCompression compression = ETextureCompression::BC7;
    ETextureFilter filter = ETextureFilter::Linear;

public:
    virtual void Serialize(core::Archive& ar) override
    {
        ar("is_srgb") << is_srgb;
        ar("generate_mips") << generate_mips;
        ar("compression") << compression;
        ar("filter") << filter;
    }
};
}
