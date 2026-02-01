#pragma once

#include "SimpleEngine/Rendering/ShaderProvider/IShaderProvider.h"
#include "SDL3/SDL_gpu.h"


namespace se::graphics
{
class PrecompiledShaderProvider : public IShaderProvider
{
public:
    /** SPIR-V 파일을 SDL_GPUShader*로 변환합니다. */
    virtual SDL_GPUShader* Provide(SDL_GPUDevice* device, const ShaderRequest& request) override;
};
}
