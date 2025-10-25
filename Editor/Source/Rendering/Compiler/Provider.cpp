#include "Rendering/Compiler/Provider.h"

#include <vector>

#include "Rendering/Compiler/Compiler.h"
#include "SimpleEngine/Core/Containers/Containers.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Gfx/ShaderUtils.h"
#include "SimpleEngine/Rendering/ShaderProvider/IShaderProvider.h"

using namespace se::rendering;


namespace se::editor::rendering
{
SDL_GPUShader* CompilingShaderProvider::Provide(SDL_GPUDevice* device, const ShaderRequest& request)
{
    const std::u8string ext = request.source_path.extension().u8string();

    // HLSL Compile
    if (ext.contains(u8".hlsl"))
    {
        Optional<vector<HLSL_Define>> defines_opt;
        if (request.hlsl_defines_opt.HasValue())
        {
            vector<HLSL_Define> hlsl_defines;
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
        return gfx::CompileFromSPIRV(device, request.source_path);
    }

    return nullptr;
}
}
