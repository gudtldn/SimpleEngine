#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::graphics
{
SE_REFLECT_ENUM(EMaterialParamType)

SE_BEGIN_REFLECT(MaterialParameterDescriptor, meta::Reflect)
    SE_REFLECT_PROPERTY(name, meta::Property)
    SE_REFLECT_PROPERTY(type, meta::Property)
    SE_REFLECT_PROPERTY(offset, meta::Property)
    SE_REFLECT_PROPERTY(default_value, meta::Property)
SE_END_REFLECT(MaterialParameterDescriptor)

uint32 MaterialParameterDescriptor::GetSize() const
{
    switch (type)
    {
        case EMaterialParamType::Float:  return 4;
        case EMaterialParamType::Float2: return 8;
        case EMaterialParamType::Float3: return 12;
        case EMaterialParamType::Float4: return 16;
    }
    return 0;
}
} // namespace se::graphics