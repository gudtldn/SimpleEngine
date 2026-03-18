#include "Graphics/Compiler/Provider.h"
#include "Graphics/Compiler/Compiler.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/ShaderUtils.h"
#include "SimpleEngine/Graphics/ShaderProvider/IShaderProvider.h"

using namespace se::graphics;


namespace se::editor
{
SDL_GPUShader* CompilingShaderProvider::Provide(graphics::RenderDevice& render_device, const ShaderRequest& request)
{
    const Optional ext_opt = request.source_path.Extension();
    if (!ext_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Shader source path has no extension: {}", request.source_path);
        return nullptr;
    }
    const String& ext = *ext_opt;

    // HLSL Compile
    if (ext.Contains(".hlsl"))
    {
        Optional<Array<HLSL_Define>> defines_opt;
        if (request.hlsl_defines_opt.HasValue())
        {
            Array<HLSL_Define> hlsl_defines;
            const auto& request_defines = request.hlsl_defines_opt.Value();

            hlsl_defines.Reserve(request_defines.Len());
            for (const auto& [name, value] : request_defines)
            {
                hlsl_defines.Emplace(name, value);
            }

            defines_opt = std::move(hlsl_defines);
        }

        return CompileFromHLSL(
            render_device,
            request.source_path,
            request.hlsl_include_dir_opt,
            defines_opt
        );
    }

    // SPIR-V Compile
    if (
        ext.Contains(".spv")
        || ext.Contains(".spirv")
        || ext.Contains(".spvt")
    )
    {
        return graphics::CompileFromSPIRV(render_device, request.source_path);
    }

    return nullptr;
}
}  // namespace se::editor
