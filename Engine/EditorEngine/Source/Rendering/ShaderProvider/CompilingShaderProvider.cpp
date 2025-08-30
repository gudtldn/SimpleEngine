module SimpleEngine.Editor.Rendering;
import :ShaderProvider.CompilingShaderProvider;

import SimpleEngine.Editor.Utility;
import SimpleEngine.Utility;
import std;


namespace se::editor::rendering::shader_provider
{
using namespace se::rendering::shader_provider;


SDL_GPUShader* CompilingShaderProvider::Provide(const ShaderRequest& request)
{
    const std::u8string ext = request.source_path.extension().u8string();

    // HLSL Compile
    if (ext.contains(u8".hlsl"))
    {
        using namespace utility::shader_utils;

        Optional<std::vector<HLSL_Define>> defines_opt;
        if (request.hlsl_defines_opt.HasValue())
        {
            std::vector<HLSL_Define> hlsl_defines;
            const auto& request_defines = request.hlsl_defines_opt.Value();

            hlsl_defines.reserve(request_defines.size());
            for (const auto& [name, value] : request_defines)
            {
                hlsl_defines.emplace_back(name, value);
            }

            defines_opt = std::move(hlsl_defines);
        }

        return CompileFromHLSL(
            device,
            request.source_path,
            request.hlsl_include_dir_opt,
            defines_opt
        );
    }

    // SPIR-V Compile
    if (
        ext.contains(u8".spv")
        || ext.contains(u8".spirv")
        || ext.contains(u8".spvt")
    )
    {
        return se::utility::shader_utils::CompileFromSPIRV(device, request.source_path);
    }

    return nullptr;
}
}
