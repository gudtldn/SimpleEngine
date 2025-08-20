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
};

/**
 * Render Graph에서 사용하는 SDL_GPUTexture Wrapper
 */
class RGTexture : public IRGResource
{
public:
    SDL_GPUTextureCreateInfo description;
    SDL_GPUTexture* actual_texture = nullptr;
};

/**
 * Render Graph에서 사용하는 SDL_GPUBuffer Wrapper
 */
class RGBuffer : public IRGResource
{
public:
    SDL_GPUBufferCreateInfo description;
    SDL_GPUBuffer* actual_buffer = nullptr;
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
