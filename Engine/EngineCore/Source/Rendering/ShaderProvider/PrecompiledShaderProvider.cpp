module SimpleEngine.Rendering;
import :ShaderProvider.PrecompiledShaderProvider;

import SimpleEngine.Utility;


namespace se::rendering::shader_provider
{
SDL_GPUShader* PrecompiledShaderProvider::Provide(SDL_GPUDevice* device, const ShaderRequest& request)
{
    if constexpr (utility::IS_DEBUG_BUILD)
    {
        const std::u8string ext = request.source_path.extension().u8string();
        if (!(
            ext.contains(u8".spv")
            || ext.contains(u8".spirv")
            || ext.contains(u8".spvt")
        ))
        {
            ConsoleLog(ELogLevel::Error, u8"Precompiled shader provider only supports SPIR-V files.");
            return nullptr;
        }
    }

    return utility::shader_utils::CompileFromSPIRV(device, request.source_path);
}
}
