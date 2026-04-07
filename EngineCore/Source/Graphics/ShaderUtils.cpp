#include "SimpleEngine/Graphics/ShaderUtils.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"


namespace se::graphics
{
SDL_GPUShader* CreateGraphicsShader(
    const RenderDevice& render_device,
    SDL_ShaderCross_ShaderStage stage,
    ArrayView<const uint8> spirv_bytecode
)
{
    const SDL_ShaderCross_SPIRV_Info spirv_info = {
        .bytecode = spirv_bytecode.Data(),
        .bytecode_size = spirv_bytecode.Len(),
        .entrypoint = "main", // DXC는 HLSL -> SPIR-V 변환 시 엔트리포인트를 항상 "main"으로 설정함.
        .shader_stage = stage,
    };

    SDL_ShaderCross_GraphicsShaderMetadata* refl_metadata =
        SDL_ShaderCross_ReflectGraphicsSPIRV(spirv_bytecode.Data(), spirv_bytecode.Len(), 0);

    if (!refl_metadata)
    {
        ConsoleLog(ELogLevel::Error, "Failed to reflect graphics shader, Err: {}", SDL_GetError());
        return nullptr;
    }

    const SDL_ShaderCross_GraphicsShaderResourceInfo resource_info = {
        .num_samplers = refl_metadata->resource_info.num_samplers,
        .num_storage_textures = refl_metadata->resource_info.num_storage_textures,
        .num_storage_buffers = refl_metadata->resource_info.num_storage_buffers,
        .num_uniform_buffers = refl_metadata->resource_info.num_uniform_buffers,
    };

    SDL_GPUShader* shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(render_device.GetRawDevice(), &spirv_info, &resource_info, 0);

    SDL_free(refl_metadata);

    if (!shader)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create graphics shader, Err: {}", SDL_GetError());
    }

    return shader;
}

SDL_GPUComputePipeline* CreateComputePipeline(
    const RenderDevice& render_device,
    ArrayView<const uint8> spirv_bytecode
)
{
    const SDL_ShaderCross_SPIRV_Info spirv_info = {
        .bytecode = spirv_bytecode.Data(),
        .bytecode_size = spirv_bytecode.Len(),
        .entrypoint = "main", // DXC는 HLSL -> SPIR-V 변환 시 엔트리포인트를 항상 "main"으로 설정함.
        .shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE,
    };

    SDL_ShaderCross_ComputePipelineMetadata* refl_metadata =
        SDL_ShaderCross_ReflectComputeSPIRV(spirv_bytecode.Data(), spirv_bytecode.Len(), 0);

    if (!refl_metadata)
    {
        ConsoleLog(ELogLevel::Error, "Failed to reflect compute shader, Err: {}", SDL_GetError());
        return nullptr;
    }

    SDL_GPUComputePipeline* pipeline =
        SDL_ShaderCross_CompileComputePipelineFromSPIRV(render_device.GetRawDevice(), &spirv_info, refl_metadata, 0);

    SDL_free(refl_metadata);

    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create compute pipeline, Err: {}", SDL_GetError());
    }

    return pipeline;
}
} // namespace se::graphics
