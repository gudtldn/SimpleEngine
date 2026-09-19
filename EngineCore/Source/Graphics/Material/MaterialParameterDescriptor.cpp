#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se
{
SE_REFLECT_ENUM_V1(EMaterialParamType)

SE_BEGIN_REFLECT_V1(MaterialParameterDescriptor, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(name, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(type, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(offset, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(default_value, meta::Reflect)
SE_END_REFLECT_V1(MaterialParameterDescriptor)

u32 MaterialParameterDescriptor::GetSize() const
{
    switch (type)
    {
        case EMaterialParamType::Uint:   return 4;
        case EMaterialParamType::Float:  return 4;
        case EMaterialParamType::Float2: return 8;
        case EMaterialParamType::Float3: return 12;
        case EMaterialParamType::Float4: return 16;
    }
    return 0;
}

u32 MaterialParameterDescriptor::GetAlignment() const
{
    // HLSL cbuffer / GLSL std140 정렬 규칙
    switch (type)
    {
        case EMaterialParamType::Uint:   return 4;
        case EMaterialParamType::Float:  return 4;
        case EMaterialParamType::Float2: return 8;
        case EMaterialParamType::Float3: return 16; // Float3는 16바이트 경계에서 시작해야 함
        case EMaterialParamType::Float4: return 16;
    }
    return 16;
}
} // namespace se
