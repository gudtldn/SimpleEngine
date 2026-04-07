#include "SimpleEngine/Graphics/ShaderUtils.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Device/RenderDevice.h"
#include "SimpleEngine/Utility/Common.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::graphics
{
namespace
{
/** SPIR-V 바이너리에서 첫 번째 OpEntryPoint의 이름을 추출합니다. */
[[nodiscard]] const char* ExtractSpvEntryPoint(ArrayView<const uint8> spirv_bytecode)
{
    static constexpr uint32 SPIRV_MAGIC = 0x07230203; // Little-endian magic
    static constexpr uint16 OP_ENTRY_POINT = 15;

    const usize byte_count = spirv_bytecode.Len();

    // 최소 헤더 크기 검증 (5 Word == 20 Byte)
    if (byte_count < 20)
    {
        return nullptr;
    }

    const uint8* data = spirv_bytecode.Data();
    const usize word_count = byte_count / sizeof(uint32);

    // memcpy를 통해 정렬에 상관없이 4바이트 word를 읽기
    // (ArrayView는 임의 포인터를 참조할 수 있으므로 정렬이 보장되지 않음)
    auto get_word = [&](usize word_idx) -> uint32
    {
        uint32 val;
        std::memcpy(&val, data + (word_idx * sizeof(uint32)), sizeof(uint32));
        return val;
    };

    if (get_word(0) != SPIRV_MAGIC)
    {
        return nullptr;
    }

    // 5워드 헤더(magic, version, generator, bound, schema)를 건너뜀
    usize i = 5;
    while (i < word_count)
    {
        const uint32 header = get_word(i);
        const uint16 opcode = static_cast<uint16>(header & 0xFFFF);
        const uint16 word_len = static_cast<uint16>(header >> 16);

        // 무한 루프 및 Out-of-Bounds 방지
        if (word_len == 0 || i + word_len > word_count)
        {
            break;
        }

        // OpEntryPoint 레이아웃: [opcode+len] [execution model] [id] [name (null-terminated, 4-byte padded)]
        // word_len >= 4: opcode(1) + execution_model(1) + id(1) + name(최소 1 word)
        if (opcode == OP_ENTRY_POINT && word_len >= 4)
        {
            const char* entry_name = reinterpret_cast<const char*>(data + ((i + 3) * sizeof(uint32)));
            const char* instruction_end = reinterpret_cast<const char*>(data + ((i + word_len) * sizeof(uint32)));

            // 문자열이 명령어 바운드 내에서 널 종료(\0)되는지 확인
            for (const char* c = entry_name; c < instruction_end; ++c)
            {
                if (*c == '\0')
                {
                    return entry_name;
                }
            }

            // \0를 찾지 못함 (손상된 SPIR-V)
            return nullptr;
        }

        i += word_len;
    }

    return nullptr;
}
} // namespace

SDL_GPUShader* CreateGraphicsShader(
    const RenderDevice& render_device,
    SDL_ShaderCross_ShaderStage stage,
    ArrayView<const uint8> spirv_bytecode
)
{
    const char* entrypoint = ExtractSpvEntryPoint(spirv_bytecode);
    if (!entrypoint)
    {
        ConsoleLog(ELogLevel::Error, "Failed to extract entry point from SPIR-V bytecode");
        return nullptr;
    }

    const SDL_ShaderCross_SPIRV_Info spirv_info = {
        .bytecode = spirv_bytecode.Data(),
        .bytecode_size = spirv_bytecode.Len(),
        .entrypoint = entrypoint,
        .shader_stage = stage,
    };

    SDL_ShaderCross_GraphicsShaderMetadata* refl_metadata =
        SDL_ShaderCross_ReflectGraphicsSPIRV(spirv_bytecode.Data(), spirv_bytecode.Len(), 0);

    SE_SCOPE_DEFER{ SDL_free(refl_metadata); };

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
    const char* entrypoint = ExtractSpvEntryPoint(spirv_bytecode);
    if (!entrypoint)
    {
        ConsoleLog(ELogLevel::Error, "Failed to extract entry point from SPIR-V bytecode");
        return nullptr;
    }

    const SDL_ShaderCross_SPIRV_Info spirv_info = {
        .bytecode = spirv_bytecode.Data(),
        .bytecode_size = spirv_bytecode.Len(),
        .entrypoint = entrypoint,
        .shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE,
    };

    SDL_ShaderCross_ComputePipelineMetadata* refl_metadata =
        SDL_ShaderCross_ReflectComputeSPIRV(spirv_bytecode.Data(), spirv_bytecode.Len(), 0);

    SE_SCOPE_DEFER{ SDL_free(refl_metadata); };

    if (!refl_metadata)
    {
        ConsoleLog(ELogLevel::Error, "Failed to reflect compute shader, Err: {}", SDL_GetError());
        return nullptr;
    }

    SDL_GPUComputePipeline* pipeline =
        SDL_ShaderCross_CompileComputePipelineFromSPIRV(render_device.GetRawDevice(), &spirv_info, refl_metadata, 0);

    if (!pipeline)
    {
        ConsoleLog(ELogLevel::Error, "Failed to create compute pipeline, Err: {}", SDL_GetError());
    }

    return pipeline;
}
} // namespace se::graphics
