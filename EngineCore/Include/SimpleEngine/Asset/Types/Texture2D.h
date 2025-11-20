#pragma once
#include "SimpleEngine/Asset/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"


namespace se::asset
{
enum class ETextureFormat
{
    None = 0,
    R8,
    R8G8B8,
    R8G8B8A8,
};

struct Texture2D : IAsset
{
    int32 width = 0;
    int32 height = 0;
    int32 channels = 0;
    ETextureFormat format = ETextureFormat::None;

    bool is_srgb = true;
    bool generate_mips = true;

    Array<uint8> pixels;
};
}
