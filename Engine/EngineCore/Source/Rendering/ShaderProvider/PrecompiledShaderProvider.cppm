export module SimpleEngine.Rendering:ShaderProvider.PrecompiledShaderProvider;
import :ShaderProvider.IShaderProvider;

import "SDL3/SDL_gpu.h";


export namespace se::rendering::shader_provider
{
class PrecompiledShaderProvider : public IShaderProvider
{
public:
    /** SPIR-V 파일을 SDL_GPUShader*로 변환합니다. */
    virtual SDL_GPUShader* Provide(SDL_GPUDevice* device, const ShaderRequest& request) override;
};
}
