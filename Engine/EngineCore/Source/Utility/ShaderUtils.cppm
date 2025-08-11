module;
#include <SDL3/SDL_gpu.h>
export module SimpleEngine.Utility:ShaderUtils;
import :FileUtils;

import SimpleEngine.Types;
import std;


export namespace se::utility::shader_utils
{
SDL_GPUShader* LoadCompiledShader(
    SDL_GPUDevice* device,
    const std::filesystem::path& shader_path,
    uint32 sampler_count,
    uint32 uniform_buffer_count,
    uint32 storage_buffer_count,
    uint32 storage_texture_count
);
}
