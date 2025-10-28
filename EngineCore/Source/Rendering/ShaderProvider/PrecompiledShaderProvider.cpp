#include "Rendering/ShaderProvider/PrecompiledShaderProvider.h"

#include "Core/Logging/Logging.h"
#include "Gfx/ShaderUtils.h"


namespace se::rendering
{
SDL_GPUShader* PrecompiledShaderProvider::Provide(SDL_GPUDevice* device, const ShaderRequest& request)
{
    if constexpr (SE_DEBUG_BUILD)
    {
        const std::u8string ext = request.source_path.extension().u8string();
        if (!(
            ext.contains(u8".spv")
            || ext.contains(u8".spirv")
            || ext.contains(u8".spvt")
        ))
        {
            ConsoleLog(ELogLevel::Error, "Precompiled shader provider only supports SPIR-V files.");
            return nullptr;
        }
    }

    return gfx::CompileFromSPIRV(device, request.source_path);
}
}
