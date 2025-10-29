#include "Rendering/ShaderProvider/PrecompiledShaderProvider.h"

#include "Core/Logging/Logging.h"
#include "Gfx/ShaderUtils.h"


namespace se::rendering
{
SDL_GPUShader* PrecompiledShaderProvider::Provide(SDL_GPUDevice* device, const ShaderRequest& request)
{
    if constexpr (SE_DEBUG_BUILD)
    {
        const std::string ext = request.source_path.extension().string();
        if (!(
            ext.contains(".spv")
            || ext.contains(".spirv")
            || ext.contains(".spvt")
        ))
        {
            ConsoleLog(ELogLevel::Error, "Precompiled shader provider only supports SPIR-V files.");
            return nullptr;
        }
    }

    return gfx::CompileFromSPIRV(device, request.source_path);
}
}
