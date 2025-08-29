export module SimpleEngine.Rendering:ShaderProvider.PrecompiledShaderProvider;
import :ShaderProvider.IShaderProvider;

import "SDL3/SDL_gpu.h";


export namespace se::rendering::shader_provider
{
class PrecompiledShaderProvider : public IShaderProvider
{
public:
    explicit PrecompiledShaderProvider(SDL_GPUDevice* device)
        : device(device)
    {
    }

    /** SPRIV 파일을 SDL_GPUShader*로 변환합니다. */
    virtual SDL_GPUShader* Provide(const ShaderRequest& request) override;

private:
    SDL_GPUDevice* device;
};
}
