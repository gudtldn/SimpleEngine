#pragma once
#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineBaseNode.h"


namespace se::asset
{
/**
 * @todo docs
 */
class PipelineTextureNode final : public PipelineBaseNode
{
public:
    struct AttributeKeys
    {
        static StringName SourceFile;
        static StringName IsSRGB;
        static StringName Compression; // e.g., "BC7", "BC5"
        static StringName Filter;      // e.g., "Nearest", "Bilinear"
        static StringName LODGroup;
    };

    [[nodiscard]] virtual refl::TypeId GetTypeId() const noexcept override;

public:
    [[nodiscard]] Optional<const String&> GetSourceFile() const;
    void SetSourceFile(const String& path);

    [[nodiscard]] bool GetSRGB() const;
    void SetSRGB(bool is_srgb);

    void SetCompression(const String& compression);
};
} // namespace se::asset
