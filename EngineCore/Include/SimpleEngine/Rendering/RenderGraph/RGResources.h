#pragma once
#include "SimpleEngine/Rendering/RenderGraph/TransientResourcePool.h"
#include "SDL3/SDL_gpu.h"


namespace se::rendering
{
class IRGResource
{
public:
    virtual ~IRGResource() = default;

    virtual void Realize(TransientResourcePool& pool) = 0;
    virtual void Unrealize(TransientResourcePool& pool) = 0;
};

class IRGTexture : public IRGResource
{
public:
    virtual ~IRGTexture() override = default;

    [[nodiscard]] SDL_GPUTexture* GetActualTexture() const { return actual_texture; }

protected:
    SDL_GPUTexture* actual_texture = nullptr;
};

class IRGBuffer : public IRGResource
{
public:
    virtual ~IRGBuffer() override = default;

    [[nodiscard]] SDL_GPUBuffer* GetActualBuffer() const { return actual_buffer; }

protected:
    SDL_GPUBuffer* actual_buffer = nullptr;
};

/**
 * Render Graph가 직접 생성하고 소유하는 임시(Transient) 텍스처
 */
class RGTransientTexture : public IRGTexture
{
public:
    virtual void Realize(TransientResourcePool& pool) override
    {
        if (!actual_texture)
        {
            actual_texture = pool.AllocateTexture(description);
        }
    }

    virtual void Unrealize(TransientResourcePool& pool) override
    {
        if (actual_texture)
        {
            pool.DeallocateTexture(description, actual_texture);
            actual_texture = nullptr;
        }
    }

public:
    SDL_GPUTextureCreateInfo description;
};

/**
 * 외부에서 Import된, Render Graph가 소유하지 않는 텍스처
 */
class RGExternalTexture : public IRGTexture
{
public:
    explicit RGExternalTexture(SDL_GPUTexture* texture)
    {
        actual_texture = texture;
    }

    virtual void Realize([[maybe_unused]] TransientResourcePool& pool) override {}
    virtual void Unrealize([[maybe_unused]] TransientResourcePool& pool) override {}
};

/**
 * Render Graph가 직접 생성하고 소유하는 임시(Transient) 버퍼
 */
class RGTransientBuffer : public IRGBuffer
{
public:
    virtual void Realize(TransientResourcePool& pool) override
    {
        if (!actual_buffer)
        {
            actual_buffer = pool.AllocateBuffer(description);
        }
    }

    virtual void Unrealize(TransientResourcePool& pool) override
    {
        if (actual_buffer)
        {
            pool.DeallocateBuffer(description, actual_buffer);
            actual_buffer = nullptr;
        }
    }

public:
    SDL_GPUBufferCreateInfo description;
};

/**
 * 외부에서 Import된, Render Graph가 소유하지 않는 텍스처
 */
class RGExternalBuffer : public IRGBuffer
{
public:
    explicit RGExternalBuffer(SDL_GPUBuffer* buffer)
    {
        actual_buffer = buffer;
    }

    virtual void Realize([[maybe_unused]] TransientResourcePool& pool) override {}
    virtual void Unrealize([[maybe_unused]] TransientResourcePool& pool) override {}
};
}
