#pragma once
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se
{
enum class ETextureFormat : uint8
{
    None = 0,
    R8,
    R8G8B8,
    R8G8B8A8,
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
    uint32 channels = 0;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    ETextureFormat format = ETextureFormat::None;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    bool is_srgb = true;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    bool generate_mips = true;

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    Array<uint8> pixels;
};
} // namespace se
