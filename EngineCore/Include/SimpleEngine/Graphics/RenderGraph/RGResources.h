#pragma once

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"
#include "SimpleEngine/Graphics/RenderGraph/FrameResourcePool.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
class SE_CORE_API RGResourceBase
{
public:
    SE_RTTI_ROOT()

    virtual ~RGResourceBase() = default;

    virtual void Realize(FrameResourcePool& pool) = 0;
    virtual void Unrealize(FrameResourcePool& pool) = 0;
};

class SE_CORE_API RGTextureBase : public RGResourceBase
{
    friend struct ::se::Registrar<RGTextureBase>;

public:
    virtual ~RGTextureBase() override = default;

    [[nodiscard]] SDL_GPUTexture* GetActualTexture() const { return actual_texture; }

protected:
    SE_ANNOTATE(actual_texture, Ignore)
    SDL_GPUTexture* actual_texture = nullptr;
};

class SE_CORE_API RGBufferBase : public RGResourceBase
{
    friend struct ::se::Registrar<RGBufferBase>;

public:
    virtual ~RGBufferBase() override = default;

    [[nodiscard]] SDL_GPUBuffer* GetActualBuffer() const { return actual_buffer; }

protected:
    SE_ANNOTATE(actual_buffer, Ignore)
    SDL_GPUBuffer* actual_buffer = nullptr;
};

/**
 * Render Graph가 직접 생성하고 소유하는 임시(Transient) 텍스처
 */
class SE_CORE_API RGTransientTexture : public RGTextureBase
{
public:
    SE_RTTI(RGTransientTexture)

    virtual void Realize(FrameResourcePool& pool) override
    {
        if (!actual_texture)
        {
            actual_texture = pool.AcquireTexture(description);
        }
    }

    virtual void Unrealize(FrameResourcePool& pool) override
    {
        if (actual_texture)
        {
            pool.ReleaseTexture(description, actual_texture);
            actual_texture = nullptr;
        }
    }

public:
    SE_ANNOTATE(description, Ignore)
    SDL_GPUTextureCreateInfo description;
};

/**
 * 외부에서 Import된, Render Graph가 소유하지 않는 텍스처
 */
class SE_CORE_API RGExternalTexture : public RGTextureBase
{
public:
    SE_RTTI(RGExternalTexture)

    explicit RGExternalTexture(SDL_GPUTexture* texture)
    {
        actual_texture = texture;
    }

    virtual void Realize([[maybe_unused]] FrameResourcePool& pool) override {}
    virtual void Unrealize([[maybe_unused]] FrameResourcePool& pool) override {}
};

/**
 * Render Graph가 직접 생성하고 소유하는 임시(Transient) 버퍼
 */
class SE_CORE_API RGTransientBuffer : public RGBufferBase
{
public:
    SE_RTTI(RGTransientBuffer)

    virtual void Realize(FrameResourcePool& pool) override
    {
        if (!actual_buffer)
        {
            actual_buffer = pool.AcquireBuffer(description);
        }
    }

    virtual void Unrealize(FrameResourcePool& pool) override
    {
        if (actual_buffer)
        {
            pool.ReleaseBuffer(description, actual_buffer);
            actual_buffer = nullptr;
        }
    }

public:
    SE_ANNOTATE(description, Ignore)
    SDL_GPUBufferCreateInfo description;
};

/**
 * 외부에서 Import된, Render Graph가 소유하지 않는 텍스처
 */
class SE_CORE_API RGExternalBuffer : public RGBufferBase
{
public:
    SE_RTTI(RGExternalBuffer)

    explicit RGExternalBuffer(SDL_GPUBuffer* buffer)
    {
        actual_buffer = buffer;
    }

    virtual void Realize([[maybe_unused]] FrameResourcePool& pool) override {}
    virtual void Unrealize([[maybe_unused]] FrameResourcePool& pool) override {}
};
} // namespace se

SE_DECLARE_REFLECTION(se::RGResourceBase, SE_CORE_API)
SE_DECLARE_REFLECTION(se::RGTextureBase, SE_CORE_API)
SE_DECLARE_REFLECTION(se::RGBufferBase, SE_CORE_API)
SE_DECLARE_REFLECTION(se::RGTransientTexture, SE_CORE_API)
SE_DECLARE_REFLECTION(se::RGExternalTexture, SE_CORE_API)
SE_DECLARE_REFLECTION(se::RGTransientBuffer, SE_CORE_API)
SE_DECLARE_REFLECTION(se::RGExternalBuffer, SE_CORE_API)
