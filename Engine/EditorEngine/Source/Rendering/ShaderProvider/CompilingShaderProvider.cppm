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
    /** HLSL 컴파일 및, SPIR-V을 가져옵니다. */
    virtual SDL_GPUShader* Provide(SDL_GPUDevice* device, const se::rendering::shader_provider::ShaderRequest& request) override;
};
}
