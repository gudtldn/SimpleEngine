#include "Asset/Loaders/Texture2DLoader.h"

#include "Asset/ImportSettings_DEPRECATED/TextureImportSettings.h"
#include "Asset/Types/Texture2D.h"
#include "Core/Logging/Logging.h"
#include "Reflection/Reflect.h"
#include "Utility/StringUtils.h"

#include "SDL3_image/SDL_image.h"


namespace se::asset
{
SE_BEGIN_REFLECT(Texture2DLoader)
SE_END_REFLECT(Texture2DLoader)

concurrency::Task<std::shared_ptr<IAsset>> Texture2DLoader::Load(
    const std::filesystem::path& physical_path,
    const IAssetImportSettings* import_settings
)
{
    TextureImportSettings final_settings;
    if (const TextureImportSettings* settings = GetSettings<TextureImportSettings>(import_settings))
    {
        final_settings = *settings;
    }

    String path_str = utility::ToString(physical_path.c_str());

    // Image를 Surface로 로드
    SDL_Surface* raw_surface = IMG_Load(path_str.CStr());
    if (!raw_surface)
    {
        ConsoleLog(ELogLevel::Error, "Failed to load image: {} (Error: {})", path_str, SDL_GetError());
        co_return nullptr;
    }

    // Texture Format 변환
    // TODO: 나중에 final_settings.compression 등을 확인하여 포맷 설정
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
    texture->channels = 4; // RGBA32는 4채널
    texture->format = ETextureFormat::R8G8B8A8;

    texture->is_srgb = final_settings.is_srgb;
    texture->generate_mips = final_settings.generate_mips;

    // 픽셀 데이터 복사
    const usize data_size = texture->width * texture->height * texture->channels;
    texture->pixels.ResizeUninitialized(data_size);

    if (SDL_MUSTLOCK(optimized_surface))
    {
        SDL_LockSurface(optimized_surface);
    }
    std::memcpy(texture->pixels.Data(), optimized_surface->pixels, data_size);
    if (SDL_MUSTLOCK(optimized_surface))
    {
        SDL_UnlockSurface(optimized_surface);
    }
    SDL_DestroySurface(optimized_surface);

    ConsoleLog(
        ELogLevel::Info,
        "Loaded Texture: {} ({}x{}, sRGB: {})",
        physical_path.filename().string(), texture->width, texture->height,
        texture->is_srgb ? "True" : "False"
    );

    co_return texture;
}
}
