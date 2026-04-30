#include "SimpleEditor/Asset/Pipeline/Factories/TextureFactory.h"
#include "SimpleEditor/Asset/Pipeline/Factories/ImageLoader.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineTextureNode.h"

#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "tracy/Tracy.hpp"

#include <utility>


namespace se::editor
{
TypeId TextureFactory::GetAssetType() const
{
    return TypeId::Get<asset::Texture2D>();
}

bool TextureFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<PipelineTextureNode>(node);
}

std::shared_ptr<asset::AssetBase> TextureFactory::CreateAsset(PipelineBaseNode* node, const PipelineImportContext& context)
{
    ZoneScopedN("TextureFactory::CreateAsset");
    std::ignore = context;

    const PipelineTextureNode* tex_node = CastChecked<const PipelineTextureNode>(node);
    const bool is_srgb = tex_node->IsSRGB();

    ImageLoadResult result = Unexpected<ImageLoadError>{ ImageLoadError::InvalidSource, "no source data" };

    // 우선순위 1: 외부 파일 경로
    if (const Optional<Path> source_file = tex_node->GetSourceFile())
    {
        result = ImageLoader::LoadFromFile(*source_file, is_srgb);
    }

    // 우선순위 2: raw pixels (Assimp mHeight > 0)
    else if (const Optional<uint64> width = tex_node->GetEmbeddedWidth())
    {
        const Optional<uint64> height = tex_node->GetEmbeddedHeight();
        const Optional<const Array<uint8>&> bytes = tex_node->GetEmbeddedBytes();

        if (height.HasValue() && bytes.HasValue())
        {
            const usize expected_size = static_cast<usize>(*width) * static_cast<usize>(*height) * 4u;
            if (*width == 0 || *height == 0 || bytes->Len() < expected_size)
            {
                ConsoleLog(
                    ELogLevel::Error,
                    "TextureFactory: Raw pixel buffer size mismatch for '{}': expected {} bytes, got {}.",
                    tex_node->GetDisplayName(), expected_size, bytes->Len()
                );
                return nullptr;
            }
            result = ImageLoader::LoadFromRawPixels(*bytes, static_cast<uint32>(*width), static_cast<uint32>(*height), is_srgb);
        }
    }

    // 우선순위 3: 압축 바이너리 (Assimp mHeight == 0)
    else if (const Optional<const Array<uint8>&> bytes = tex_node->GetEmbeddedBytes())
    {
        const StringView format_hint = tex_node->GetEmbeddedFormat()
            .Map([](const String& s) { return s.Bytes(); })
            .ValueOrDefault();

        result = ImageLoader::LoadFromMemory(*bytes, is_srgb, format_hint);
    }

    else
    {
        ConsoleLog(ELogLevel::Error, "TextureFactory: Node '{}' has no source data.", tex_node->GetDisplayName());
        return nullptr;
    }

    if (result.HasError())
    {
        ConsoleLog(ELogLevel::Warning, "TextureFactory: Image load failed for node '{}': {}", tex_node->GetDisplayName(), result.Error().What());
        return nullptr;
    }

    auto texture = std::make_shared<asset::Texture2D>();
    texture->width = result->width;
    texture->height = result->height;
    texture->format = result->format;
    texture->generate_mips = true;
    texture->pixels = std::move(result->pixels);
    return texture;
}
} // namespace se::editor
