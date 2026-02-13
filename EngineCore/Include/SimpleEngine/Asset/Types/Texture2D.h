#pragma once
#include "SimpleEngine/Asset/Types/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se::asset
{
enum class ETextureFormat : uint8
{
    None = 0,
    R8,
    R8G8B8,
    R8G8B8A8,
};

class SE_CORE_API Texture2D : public IAsset
{
    SE_CLASS(Texture2D, IAsset)

public:
    SE_PROPERTY(=::se::meta::ReadOnly)
    uint32 width = 0;

    SE_PROPERTY(=::se::meta::ReadOnly)
    uint32 height = 0;

    SE_PROPERTY(=::se::meta::ReadOnly)
    uint32 channels = 0;

    SE_PROPERTY(=::se::meta::ReadOnly)
    ETextureFormat format = ETextureFormat::None;

    SE_PROPERTY(=::se::meta::ReadOnly)
    bool is_srgb = true;

    SE_PROPERTY(=::se::meta::ReadOnly)
    bool generate_mips = true;

    SE_PROPERTY(=::se::meta::ReadOnly)
    Array<uint8> pixels;
};
}  // namespace se::asset
