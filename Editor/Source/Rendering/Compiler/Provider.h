#pragma once
#include "SimpleEngine/Rendering/ShaderProvider/IShaderProvider.h"
#include "SDL3/SDL_gpu.h"


namespace se::editor::rendering
{
/**
 * HLSL 컴파일 및, SPIR-V을 가져오는 클래스
 */
class CompilingShaderProvider : public se::rendering::IShaderProvider
{
public:
    /** HLSL 컴파일 및, SPIR-V을 가져옵니다. */
    virtual SDL_GPUShader* Provide(SDL_GPUDevice* device, const se::rendering::ShaderRequest& request) override;
};
}
