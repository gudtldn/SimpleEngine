#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineTextureNode.h"

#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
SE_BEGIN_REFLECT(PipelineTextureNode, meta::Reflect, meta::Hidden, meta::Transient)
SE_END_REFLECT(PipelineTextureNode)

Optional<Path> PipelineTextureNode::GetSourceFile() const
{
    return attributes.GetAttribute<String>(Keys::SOURCE_FILE)
                     .Map([](const String& s) { return Path{ s }; });
}

void PipelineTextureNode::SetSourceFile(const Path& file_path)
{
    attributes.SetAttribute(Keys::SOURCE_FILE, file_path.ToString());
}

bool PipelineTextureNode::IsSRGB() const
{
    // 기본값 true: Albedo/Diffuse 텍스처는 sRGB가 일반적
    // Normal/Mask/Roughness 등 선형 데이터 텍스처는 임포트 시 SetSRGB(false)를 명시적으로 호출하도록 함
    return attributes.GetAttribute<bool>(Keys::USE_SRGB).ValueOr(true);
}

void PipelineTextureNode::SetSRGB(bool is_srgb)
{
    attributes.SetAttribute(Keys::USE_SRGB, is_srgb);
}

Optional<const String&> PipelineTextureNode::GetCompression() const
{
    return attributes.GetAttribute<String>(Keys::COMPRESSION);
}

void PipelineTextureNode::SetCompression(const String& compression)
{
    attributes.SetAttribute(Keys::COMPRESSION, compression);
}

Optional<ArrayView<const u8>> PipelineTextureNode::GetEmbeddedBytes() const
{
    if (embedded_bytes.IsEmpty())
    {
        return {};
    }
    return ArrayView<const u8>{ embedded_bytes };
}

void PipelineTextureNode::SetEmbeddedBytes(ArrayView<const u8> bytes)
{
    embedded_bytes.ResizeUninitialized(bytes.Len());
    std::memcpy(embedded_bytes.Data(), bytes.Data(), bytes.Len());
}

Optional<const String&> PipelineTextureNode::GetEmbeddedFormat() const
{
    return attributes.GetAttribute<String>(Keys::EMBEDDED_FORMAT);
}

void PipelineTextureNode::SetEmbeddedFormat(const String& format)
{
    attributes.SetAttribute(Keys::EMBEDDED_FORMAT, format);
}

Optional<u64> PipelineTextureNode::GetEmbeddedWidth() const
{
    return attributes.GetAttribute<u64>(Keys::EMBEDDED_WIDTH);
}

void PipelineTextureNode::SetEmbeddedWidth(u64 width)
{
    attributes.SetAttribute(Keys::EMBEDDED_WIDTH, width);
}

Optional<u64> PipelineTextureNode::GetEmbeddedHeight() const
{
    return attributes.GetAttribute<u64>(Keys::EMBEDDED_HEIGHT);
}

void PipelineTextureNode::SetEmbeddedHeight(u64 height)
{
    attributes.SetAttribute(Keys::EMBEDDED_HEIGHT, height);
}
} // namespace se::editor
