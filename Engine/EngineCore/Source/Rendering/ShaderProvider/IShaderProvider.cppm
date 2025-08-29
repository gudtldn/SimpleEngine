export module SimpleEngine.Rendering:ShaderProvider.IShaderProvider;

import SimpleEngine.Types;
import std;

import "SDL3/SDL_gpu.h";


export namespace se::rendering::shader_provider
{
/**
 * Shader 요청에 필요한 정보
 */
struct ShaderRequest
{
    const std::filesystem::path& source_path;

    // HLSL 컴파일 시 사용
    Optional<const std::filesystem::path&> hlsl_include_dir_opt = std::nullopt;
    Optional<const std::vector<std::pair<const char*, const char*>>&> hlsl_defines_opt = std::nullopt;
};


/**
 * ShaderManager에 SDL_GPUShader* 를 제공하는 인터페이스
 */
class IShaderProvider
{
public:
    virtual ~IShaderProvider() = default;

    /** 주어진 Request에 따라 Shader를 가져옵니다. */
    virtual SDL_GPUShader* Provide(const ShaderRequest& request) = 0;
};
}
