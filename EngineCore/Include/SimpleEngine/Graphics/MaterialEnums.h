#pragma once
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::graphics
{
enum class EBlendMode : uint8
{
    Opaque,
    Masked,
    Translucent,
    Additive,
    Modulate
};

enum class EShadingModel : uint8
{
    Lit,
    Unlit,
    Subsurface,
    ClearCoat,
};
} // namespace se::graphics
