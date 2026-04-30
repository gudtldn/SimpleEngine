#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Types/Path.h"


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
        inline static const StringName SOURCE_FILE     = "SourceFile";
        inline static const StringName USE_SRGB        = "UseSRGB";

        // embedded 텍스처 전용
        inline static const StringName EMBEDDED_BYTES  = "EmbeddedBytes";   // Array<uint8>: 압축 바이트 (mHeight==0) 또는 RGBA8 pixels (mHeight>0)
        inline static const StringName EMBEDDED_FORMAT = "EmbeddedFormat";  // String: "png", "jpg", "tga", ... (압축 바이트일 때만 사용)
        inline static const StringName EMBEDDED_WIDTH  = "EmbeddedWidth";   // uint64: raw pixel width (mHeight>0일 때만 사용)
        inline static const StringName EMBEDDED_HEIGHT = "EmbeddedHeight";  // uint64: raw pixel height (mHeight>0일 때만 사용)
    };

public:
    [[nodiscard]] Optional<Path> GetSourceFile() const;
    void SetSourceFile(const Path& file_path);

    [[nodiscard]] bool IsSRGB() const;
    void SetSRGB(bool is_srgb);

    [[nodiscard]] Optional<const Array<uint8>&> GetEmbeddedBytes() const;
    void SetEmbeddedBytes(Array<uint8> bytes);

    [[nodiscard]] Optional<const String&> GetEmbeddedFormat() const;
    void SetEmbeddedFormat(const String& format);

    [[nodiscard]] Optional<uint64> GetEmbeddedWidth() const;
    void SetEmbeddedWidth(uint64 width);

    [[nodiscard]] Optional<uint64> GetEmbeddedHeight() const;
    void SetEmbeddedHeight(uint64 height);
};
} // namespace se::editor
