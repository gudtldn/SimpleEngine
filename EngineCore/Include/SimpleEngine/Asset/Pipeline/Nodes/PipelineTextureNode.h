#pragma once
#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineBaseNode.h"


namespace se::asset
{
/**
 * Texture의 원본 파일 정보 및 임포트 설정을 담당하는 노드
 */
class PipelineTextureNode final : public PipelineBaseNode
{
public:
    struct Keys
    {
        inline static const StringName SOURCE_FILE = "SourceFile";
        inline static const StringName USE_SRGB = "UseSRGB";
        inline static const StringName COMPRESSION = "Compression"; // e.g., "BC7", "BC5"
        inline static const StringName FILTER = "Filter";           // e.g., "Nearest", "Bilinear"
    };

    [[nodiscard]] virtual refl::TypeId GetTypeId() const noexcept override;

public:
    [[nodiscard]] Optional<const String&> GetSourceFile() const;
    void SetSourceFile(const String& path);

    [[nodiscard]] bool IsSRGB() const;
    void SetSRGB(bool is_srgb);

    [[nodiscard]] Optional<const String&> GetCompression() const;
    void SetCompression(const String& compression);
};
} // namespace se::asset
