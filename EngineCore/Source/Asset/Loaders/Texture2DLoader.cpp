#include "Asset/Loaders/Texture2DLoader.h"

#include "Asset/Types/Texture2D.h"
#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"

#include "SDL3_image/SDL_image.h"


namespace se::asset
{
concurrency::Task<std::shared_ptr<IAsset>> Texture2DLoader::Load(const std::filesystem::path& physical_path)
{
    String path_str = utility::string::ToString(physical_path.c_str());

    // Image를 Surface로 로드
    SDL_Surface* raw_surface = IMG_Load(path_str.CStr());
    if (!raw_surface)
    {
        ConsoleLog(ELogLevel::Error, "Failed to load image: {} (Error: {})", path_str, SDL_GetError());
        co_return nullptr;
    }

    // Texture Format 변환
    SDL_Surface* optimized_surface = SDL_ConvertSurface(raw_surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw_surface);
    if (!optimized_surface)
    {
        ConsoleLog(ELogLevel::Error, "Failed to convert image to RGBA32: {}", path_str);
        co_return nullptr;
    }

    // Texture2D로 복사
    auto texture = std::make_shared<Texture2D>();
    texture->width = optimized_surface->w;
    texture->height = optimized_surface->h;
    texture->channels = 4;
    texture->format = ETextureFormat::R8G8B8A8;

    // 픽셀 데이터 복사
    const size_t data_size = texture->width * texture->height * texture->channels;
    texture->pixels.Resize(data_size);

    std::memcpy(texture->pixels.Data(), optimized_surface->pixels, data_size);
    SDL_DestroySurface(optimized_surface);

    ConsoleLog(ELogLevel::Info, "Loaded Texture: {} ({}x{})", physical_path.filename().string(), texture->width, texture->height);
    co_return texture;
}
}
