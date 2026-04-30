#include "SimpleEditor/Asset/Pipeline/Factories/ImageLoader.h"

#include "SDL3_image/SDL_image.h"
#include "SDL3/SDL.h"


namespace se::editor
{
namespace
{
/** SDL_Surface를 RGBA8로 변환하고 픽셀을 ImageData로 패키징합니다. */
ImageLoadResult PackSurface(SDL_Surface* in_surface, bool is_srgb)
{
    if (!in_surface)
    {
        return Unexpected<ImageLoadError>{ ImageLoadError::InvalidSource, "null surface" };
    }

    SDL_Surface* rgba = SDL_ConvertSurface(in_surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(in_surface);

    if (!rgba)
    {
        return Unexpected<ImageLoadError>{ ImageLoadError::ConversionFailed, SDL_GetError() };
    }

    const uint32 width = static_cast<uint32>(rgba->w);
    const uint32 height = static_cast<uint32>(rgba->h);
    const usize size = static_cast<usize>(width) * height * 4u;

    ImageData result;
    result.width = width;
    result.height = height;
    result.format = is_srgb ? asset::ETextureFormat::R8G8B8A8_UNORM_SRGB : asset::ETextureFormat::R8G8B8A8_UNORM;
    result.pixels.ResizeUninitialized(size);
    std::memcpy(result.pixels.Data(), rgba->pixels, size);

    SDL_DestroySurface(rgba);
    return result;
}
} // namespace

ImageLoadResult ImageLoader::LoadFromFile(const Path& file_path, bool is_srgb)
{
    SDL_Surface* surf = IMG_Load(file_path.CStr());
    if (!surf)
    {
        return Unexpected<ImageLoadError>{ ImageLoadError::LoadFailed, SDL_GetError() };
    }
    return PackSurface(surf, is_srgb);
}

ImageLoadResult ImageLoader::LoadFromMemory(ArrayView<const uint8> data, bool is_srgb, StringView format_hint)
{
    SDL_IOStream* io = SDL_IOFromConstMem(data.Data(), static_cast<size_t>(data.Len()));
    if (!io)
    {
        return Unexpected<ImageLoadError>{ ImageLoadError::IOError, SDL_GetError() };
    }

    SDL_Surface* surf = format_hint.IsEmpty()
                            ? IMG_Load_IO(io, true)
                            : IMG_LoadTyped_IO(io, true, format_hint.Data());

    if (!surf)
    {
        return Unexpected<ImageLoadError>{ ImageLoadError::LoadFailed, SDL_GetError() };
    }
    return PackSurface(surf, is_srgb);
}

ImageData ImageLoader::LoadFromRawPixels(ArrayView<const uint8> rgba8_pixels, uint32 width, uint32 height, bool is_srgb)
{
    const usize size = static_cast<usize>(width) * height * 4u;
    SE_ASSERT(rgba8_pixels.Len() >= size, "Raw pixel buffer size is smaller than expected.");

    ImageData result;
    result.width = width;
    result.height = height;
    result.format = is_srgb ? asset::ETextureFormat::R8G8B8A8_UNORM_SRGB : asset::ETextureFormat::R8G8B8A8_UNORM;
    result.pixels.ResizeUninitialized(size);
    std::memcpy(result.pixels.Data(), rgba8_pixels.Data(), size);
    return result;
}

} // namespace se::editor
