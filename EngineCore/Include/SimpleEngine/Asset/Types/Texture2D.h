#pragma once
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
/** GPU 텍스처 포맷 (SDL_GPUTextureFormat과 1:1 대응) */
enum class ETextureFormat : uint8
{
    None,

    // 비압축 선형
    R8_UNORM,
    R8G8_UNORM,
    R8G8B8A8_UNORM,

    // 비압축 sRGB
    R8G8B8A8_UNORM_SRGB,

    // 비압축 부동소수점
    R16G16_FLOAT,
    R16G16B16A16_FLOAT,
    R11G11B10_UFLOAT,

    // BC 압축 선형
    BC1_UNORM,
    BC3_UNORM,
    BC4_UNORM,
    BC5_UNORM,
    BC7_UNORM,

    // BC 압축 sRGB
    BC1_UNORM_SRGB,
    BC3_UNORM_SRGB,
    BC7_UNORM_SRGB,
};

/** ETextureFormat 포맷이 GPU 블록 압축(BCn)인지 반환합니다. */
constexpr bool IsCompressed(ETextureFormat fmt) noexcept
{
    switch (fmt)
    {
    case ETextureFormat::BC1_UNORM:
    case ETextureFormat::BC1_UNORM_SRGB:
    case ETextureFormat::BC3_UNORM:
    case ETextureFormat::BC3_UNORM_SRGB:
    case ETextureFormat::BC4_UNORM:
    case ETextureFormat::BC5_UNORM:
    case ETextureFormat::BC7_UNORM:
    case ETextureFormat::BC7_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}

/** ETextureFormat 포맷이 sRGB 감마 인코딩인지 반환합니다. */
constexpr bool IsSRGB(ETextureFormat fmt) noexcept
{
    switch (fmt)
    {
    case ETextureFormat::R8G8B8A8_UNORM_SRGB:
    case ETextureFormat::BC1_UNORM_SRGB:
    case ETextureFormat::BC3_UNORM_SRGB:
    case ETextureFormat::BC7_UNORM_SRGB:
        return true;
    default:
        return false;
    }
}

/**
 * 비압축 포맷의 픽셀당 바이트 수를 반환합니다.
 * BCn 압축 포맷은 0을 반환합니다. IsCompressed()와 함께 사용하세요.
 */
constexpr uint32 GetBytesPerPixel(ETextureFormat fmt) noexcept
{
    switch (fmt)
    {
    case ETextureFormat::R8_UNORM:              return 1;
    case ETextureFormat::R8G8_UNORM:            return 2;
    case ETextureFormat::R8G8B8A8_UNORM:
    case ETextureFormat::R8G8B8A8_UNORM_SRGB:
    case ETextureFormat::R11G11B10_UFLOAT:      return 4;
    case ETextureFormat::R16G16_FLOAT:          return 4;
    case ETextureFormat::R16G16B16A16_FLOAT:    return 8;
    default:                                    return 0;
    }
}

/**
 * BCn 압축 포맷의 4×4 블록당 바이트 수를 반환합니다.
 * 비압축 포맷은 0을 반환합니다. IsCompressed()와 함께 사용하세요.
 */
constexpr uint32 GetBlockByteSize(ETextureFormat fmt) noexcept
{
    switch (fmt)
    {
    case ETextureFormat::BC1_UNORM:
    case ETextureFormat::BC1_UNORM_SRGB:
    case ETextureFormat::BC4_UNORM:
        return 8;
    case ETextureFormat::BC3_UNORM:
    case ETextureFormat::BC3_UNORM_SRGB:
    case ETextureFormat::BC5_UNORM:
    case ETextureFormat::BC7_UNORM:
    case ETextureFormat::BC7_UNORM_SRGB:
        return 16;
    default:
        return 0;
    }
}

/** pixels 배열 내 개별 밉 레벨의 위치와 크기 */
struct SE_ANNOTATION(=meta::Reflect) MipDescriptor
{
    SE_ANNOTATION(=meta::Property)
    uint32 offset = 0;

    SE_ANNOTATION(=meta::Property)
    uint32 size = 0;

    SE_ANNOTATION(=meta::Property)
    uint32 width = 0;

    SE_ANNOTATION(=meta::Property)
    uint32 height = 0;
};

class SE_CORE_API SE_ANNOTATION(=meta::Reflect) Texture2D : public AssetBase
{
    SE_CLASS(Texture2D, AssetBase)

public:
    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    uint32 width = 0;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    uint32 height = 0;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    ETextureFormat format = ETextureFormat::None;

    /**
     * GPU 측 밉맵 자동 생성 여부
     * mips가 비어있지 않거나 IsCompressed(format)이면 무시됩니다.
     */
    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    bool generate_mips = true;

    /**
     * 밉 레벨 위치 정보
     * 비어있으면: pixels가 밉 0만 보유합니다.
     * 있으면: pixels는 각 MipDescriptor의 offset/size 기준 연속 데이터입니다.
     */
    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    Array<MipDescriptor> mips;

    /**
     * 픽셀 데이터 (row-tightly-packed, 패딩 없음, format 기준)
     * mips가 비어있으면: width × height × GetBytesPerPixel(format) Byte
     * mips가 있으면: 각 밉 레벨 데이터의 연속 배열
     */
    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    Array<uint8> pixels;
};
} // namespace se

SE_DECLARE_REFLECTION(se::MipDescriptor)
