export module SimpleEngine.Editor.Rendering:ShaderProvider.CompilingShaderProvider;

import SimpleEngine.Rendering;

import "SDL3/SDL_gpu.h";


export namespace se::editor::rendering::shader_provider
{
/**
 * HLSL 컴파일 및, SPIR-V을 가져오는 클래스
 */
class CompilingShaderProvider : public se::rendering::shader_provider::IShaderProvider
{
public:
    explicit CompilingShaderProvider(SDL_GPUDevice* in_device)
        : device(in_device)
    {
    }

    virtual SDL_GPUShader* Provide(const se::rendering::shader_provider::ShaderRequest& request) override;

private:
    SDL_GPUDevice* device;
};
}
