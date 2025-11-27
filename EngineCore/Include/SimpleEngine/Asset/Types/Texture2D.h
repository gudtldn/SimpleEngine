#pragma once
#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se::asset
{
enum class ETextureFormat
{
    None = 0,
    R8,
    R8G8B8,
    R8G8B8A8,
};

struct SE_CORE_API Texture2D : IAsset
{
    SE_PROPERTY(=meta::ReadOnly)
    int32 width = 0;

    SE_PROPERTY(=meta::ReadOnly)
    int32 height = 0;

    SE_PROPERTY(=meta::ReadOnly)
    int32 channels = 0;

    SE_PROPERTY(=meta::ReadOnly)
    ETextureFormat format = ETextureFormat::None;

    SE_PROPERTY(=meta::ReadOnly)
    bool is_srgb = true;

    SE_PROPERTY(=meta::ReadOnly)
    bool generate_mips = true;

    SE_PROPERTY(=meta::ReadOnly)
    Array<uint8> pixels;
};
}
