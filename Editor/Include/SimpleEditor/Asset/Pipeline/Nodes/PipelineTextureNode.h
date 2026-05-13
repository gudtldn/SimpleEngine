#pragma once

#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineBaseNode.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
class Path;
}

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
        inline static const StringName SOURCE_FILE     = "SourceFile";      // String: 외부 파일 경로
        inline static const StringName USE_SRGB        = "UseSRGB";         // bool: sRGB 여부
        inline static const StringName COMPRESSION     = "Compression";     // String: "None","BC7","BC5"

        // embedded 텍스처 전용
        inline static const StringName EMBEDDED_BYTES  = "EmbeddedBytes";   // Array<u8>: 압축 바이트 (mHeight==0) 또는 RGBA8 pixels (mHeight>0)
        inline static const StringName EMBEDDED_FORMAT = "EmbeddedFormat";  // String: "png", "jpg", "tga", ... (압축 바이트일 때만 사용)
        inline static const StringName EMBEDDED_WIDTH  = "EmbeddedWidth";   // u64: raw pixel width (mHeight>0일 때만 사용)
        inline static const StringName EMBEDDED_HEIGHT = "EmbeddedHeight";  // u64: raw pixel height (mHeight>0일 때만 사용)
    };

public:
    [[nodiscard]] Optional<Path> GetSourceFile() const;
    void SetSourceFile(const Path& file_path);

    [[nodiscard]] bool IsSRGB() const;
    void SetSRGB(bool is_srgb);

    [[nodiscard]] Optional<const String&> GetCompression() const;
    void SetCompression(const String& compression);

    [[nodiscard]] Optional<ArrayView<const u8>> GetEmbeddedBytes() const;
    void SetEmbeddedBytes(ArrayView<const u8> bytes);

    [[nodiscard]] Optional<const String&> GetEmbeddedFormat() const;
    void SetEmbeddedFormat(const String& format);

    [[nodiscard]] Optional<u64> GetEmbeddedWidth() const;
    void SetEmbeddedWidth(u64 width);

    [[nodiscard]] Optional<u64> GetEmbeddedHeight() const;
    void SetEmbeddedHeight(u64 height);

private:
    Array<u8> embedded_bytes;
};
} // namespace se::editor
