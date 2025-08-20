export module SimpleEngine.Rendering:RenderGraph.RGResources;

import SimpleEngine.Types;
import std;

import <SDL3/SDL_gpu.h>;


export namespace se::rendering::render_graph
{
class IRGResource
{
public:
    virtual ~IRGResource() = default;

    virtual void Realize(SDL_GPUDevice* device) = 0;
    virtual void Unrealize(SDL_GPUDevice* device) = 0;
};

/**
 * Render Graph에서 사용하는 SDL_GPUTexture Wrapper
 */
class RGTexture : public IRGResource
{
public:
    virtual void Realize(SDL_GPUDevice* device) override
    {
        actual_texture = SDL_CreateGPUTexture(device, &description);
    }

    virtual void Unrealize(SDL_GPUDevice* device) override
    {
        if (actual_texture)
        {
            SDL_ReleaseGPUTexture(device, actual_texture);
            actual_texture = nullptr;
        }
    }

public:
    SDL_GPUTexture* actual_texture = nullptr;
    SDL_GPUTextureCreateInfo description;
};

/**
 * Render Graph에서 사용하는 SDL_GPUBuffer Wrapper
 */
class RGBuffer : public IRGResource
{
public:
    virtual void Realize(SDL_GPUDevice* device) override
    {
        actual_buffer = SDL_CreateGPUBuffer(device, &description);
    }

    virtual void Unrealize(SDL_GPUDevice* device) override
    {
        if (actual_buffer)
        {
            SDL_ReleaseGPUBuffer(device, actual_buffer);
            actual_buffer = nullptr;
        }
    }

public:
    SDL_GPUBuffer* actual_buffer = nullptr;
    SDL_GPUBufferCreateInfo description;
};
}


enum class [[deprecated]] ERenderPassType : uint8
{
    Unknown = 0,
    ShadowMap,
    GBuffer,
    Lighting,
    Forward,
    PostProcess,
    UI
};
