#include "Graphics/ShaderProvider/PrecompiledShaderProvider.h"

#include "Core/Logging/Logging.h"
#include "Graphics/ShaderUtils.h"


namespace se::graphics
{
SDL_GPUShader* PrecompiledShaderProvider::Provide(SDL_GPUDevice* device, const ShaderRequest& request)
{
    if constexpr (SE_DEBUG_BUILD)
    {
        const Optional ext_opt = request.source_path.Extension();
        if (!ext_opt.HasValue())
        {
            ConsoleLog(ELogLevel::Error, "Shader file has no extension: {}", request.source_path);
            return nullptr;
        }

        const String& ext = *ext_opt;
        if (!(
            ext.Contains(".spv")
            || ext.Contains(".spirv")
            || ext.Contains(".spvt")
        ))
        {
            ConsoleLog(ELogLevel::Error, "Precompiled shader provider only supports SPIR-V files.");
            return nullptr;
        }
    }

    return graphics::CompileFromSPIRV(device, request.source_path);
}
}  // namespace se::graphics
