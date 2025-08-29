module SimpleEngine.Rendering;
import :ShaderProvider.PrecompiledShaderProvider;

import SimpleEngine.Utility;


namespace se::rendering::shader_provider
{
SDL_GPUShader* PrecompiledShaderProvider::Provide(const ShaderRequest& request)
{
    return utility::shader_utils::CompileFromSPIRV(device, request.source_path);
}
}
