#include "SimpleEditor/Asset/Pipeline/Factories/Texture2DFactory.h"
#include "SimpleEditor/Asset/Pipeline/Factories/ImageLoader.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineTextureNode.h"

#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
TypeId Texture2DFactory::GetAssetType() const
{
    return TypeId::Of<Texture2D>();
}

bool Texture2DFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<PipelineTextureNode>(node);
}

std::shared_ptr<AssetBase> Texture2DFactory::CreateAsset(PipelineBaseNode* node, const PipelineImportContext& context)
{
    ZoneScopedN("Texture2DFactory::CreateAsset");
    std::ignore = context;

    const PipelineTextureNode* tex_node = CastChecked<const PipelineTextureNode>(node);
    const bool is_srgb = tex_node->IsSRGB();

    ImageLoadResult load_result = [&]() -> ImageLoadResult
    {
        // 우선순위 1: 외부 파일 경로
        if (const auto source_file = tex_node->GetSourceFile())
        {
            return ImageLoader::LoadFromFile(*source_file, is_srgb);
        }

        if (const auto embedded = tex_node->GetEmbeddedBytes())
        {
            // 이미 디코딩된 RGBA8 raw pixels (Assimp mHeight > 0)
            const auto width = tex_node->GetEmbeddedWidth();
            const auto height = tex_node->GetEmbeddedHeight();

            if (width && height && *width > 0 && *height > 0)
            {
                const usize expected_size = static_cast<usize>(*width) * static_cast<usize>(*height) * 4u;
                if (expected_size <= embedded->Len())
                {
                    return ImageLoader::LoadFromRawPixels(
                        *embedded,
                        static_cast<u32>(*width),
                        static_cast<u32>(*height),
                        is_srgb
                    );
                }

                return Unexpected<ImageLoadError>{
                    ImageLoadError::LoadFailed,
                    String::Format(
                        "Texture2DFactory: Raw pixel buffer size mismatch for '{}': expected {} bytes, got {}.",
                        tex_node->GetDisplayName(), expected_size, embedded->Len()
                    )
                };
            }

            // PNG/JPG/TGA 같은 압축 바이너리 (Assimp mHeight == 0)
            const auto opt_format = tex_node->GetEmbeddedFormat();
            const StringView format_hint = opt_format ? opt_format->Bytes() : StringView{};

            return ImageLoader::LoadFromMemory(*embedded, is_srgb, format_hint);
        }

        return Unexpected<ImageLoadError>{
            ImageLoadError::InvalidSource,
            String::Format("Texture2DFactory: Node '{}' has no source data.", tex_node->GetDisplayName())
        };
    }();

    if (load_result.HasError())
    {
        ConsoleLog(ELogLevel::Error, "Texture2DFactory: Failed to load '{}': {}", tex_node->GetDisplayName(), load_result.Error().What());
        return nullptr;
    }

    auto texture = std::make_shared<Texture2D>();
    texture->width = load_result->width;
    texture->height = load_result->height;
    texture->format = load_result->format;
    texture->generate_mips = true;
    texture->pixels = std::move(load_result->pixels);
    return texture;
}
} // namespace se::editor
