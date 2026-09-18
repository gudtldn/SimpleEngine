#pragma once

#include "../../Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"
#include "SimpleEngine/Graphics/RenderGraph/FrameResourcePool.h"

#include "SDL3/SDL_gpu.h"


namespace se
{
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGResourceBase
{
    SE_CLASS_V1(RGResourceBase)

public:
    SE_RTTI_ROOT()

    virtual ~RGResourceBase() = default;

    virtual void Realize(FrameResourcePool& pool) = 0;
    virtual void Unrealize(FrameResourcePool& pool) = 0;
};

class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGTextureBase : public RGResourceBase
{
    SE_CLASS_V1(RGTextureBase, RGResourceBase)

public:
    virtual ~RGTextureBase() override = default;

    [[nodiscard]] SDL_GPUTexture* GetActualTexture() const { return actual_texture; }

protected:
    SDL_GPUTexture* actual_texture = nullptr;
};

class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGBufferBase : public RGResourceBase
{
    SE_CLASS_V1(RGBufferBase, RGResourceBase)

public:
    virtual ~RGBufferBase() override = default;

    [[nodiscard]] SDL_GPUBuffer* GetActualBuffer() const { return actual_buffer; }

protected:
    SDL_GPUBuffer* actual_buffer = nullptr;
};

/**
 * Render Graph가 직접 생성하고 소유하는 임시(Transient) 텍스처
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGTransientTexture : public RGTextureBase
{
    SE_CLASS_V1(RGTransientTexture, RGTextureBase)

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
    SDL_GPUTextureCreateInfo description;
};

/**
 * 외부에서 Import된, Render Graph가 소유하지 않는 텍스처
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGExternalTexture : public RGTextureBase
{
    SE_CLASS_V1(RGExternalTexture, RGTextureBase)

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
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGTransientBuffer : public RGBufferBase
{
    SE_CLASS_V1(RGTransientBuffer, RGBufferBase)

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
    SDL_GPUBufferCreateInfo description;
};

/**
 * 외부에서 Import된, Render Graph가 소유하지 않는 텍스처
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) RGExternalBuffer : public RGBufferBase
{
    SE_CLASS_V1(RGExternalBuffer, RGBufferBase)

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

SE_DECLARE_REFLECTION(se::RGResourceBase)
SE_DECLARE_REFLECTION(se::RGTextureBase)
SE_DECLARE_REFLECTION(se::RGBufferBase)
SE_DECLARE_REFLECTION(se::RGTransientTexture)
SE_DECLARE_REFLECTION(se::RGExternalTexture)
SE_DECLARE_REFLECTION(se::RGTransientBuffer)
SE_DECLARE_REFLECTION(se::RGExternalBuffer)
