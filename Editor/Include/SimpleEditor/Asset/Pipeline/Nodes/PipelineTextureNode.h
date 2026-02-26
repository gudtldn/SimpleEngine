#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
/**
 * Texture의 원본 파일 정보 및 임포트 설정을 담당하는 노드
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) PipelineTextureNode final : public PipelineBaseNode
{
    SE_CLASS(PipelineTextureNode, PipelineBaseNode)

public:
    struct Keys
    {
        inline static const StringName SOURCE_FILE = "SourceFile";
        inline static const StringName USE_SRGB = "UseSRGB";
        inline static const StringName COMPRESSION = "Compression"; // e.g., "BC7", "BC5"
        inline static const StringName FILTER = "Filter";           // e.g., "Nearest", "Bilinear"
    };

public:
    // TODO: 반환값 fs::path로 할지 고민
    [[nodiscard]] Optional<const String&> GetSourceFile() const;
    void SetSourceFile(const String& file_path);

    [[nodiscard]] bool IsSRGB() const;
    void SetSRGB(bool is_srgb);

    [[nodiscard]] Optional<const String&> GetCompression() const;
    void SetCompression(const String& compression);
};
} // namespace se::editor
