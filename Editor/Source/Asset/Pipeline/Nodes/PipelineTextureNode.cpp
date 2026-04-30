#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineTextureNode.h"


namespace se::editor
{
SE_BEGIN_REFLECT(PipelineTextureNode, meta::Internal)
SE_END_REFLECT(PipelineTextureNode)

Optional<Path> PipelineTextureNode::GetSourceFile() const
{
    return attributes.GetAttribute<String>(Keys::SOURCE_FILE)
        .Map([](const String& s) { return Path(s); });
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

Optional<const Array<uint8>&> PipelineTextureNode::GetEmbeddedBytes() const
{
    return attributes.GetAttribute<Array<uint8>>(Keys::EMBEDDED_BYTES);
}

void PipelineTextureNode::SetEmbeddedBytes(Array<uint8> bytes)
{
    attributes.SetAttribute(Keys::EMBEDDED_BYTES, std::move(bytes));
}

Optional<const String&> PipelineTextureNode::GetEmbeddedFormat() const
{
    return attributes.GetAttribute<String>(Keys::EMBEDDED_FORMAT);
}

void PipelineTextureNode::SetEmbeddedFormat(const String& format)
{
    attributes.SetAttribute(Keys::EMBEDDED_FORMAT, format);
}

Optional<uint64> PipelineTextureNode::GetEmbeddedWidth() const
{
    return attributes.GetAttribute<uint64>(Keys::EMBEDDED_WIDTH);
}

void PipelineTextureNode::SetEmbeddedWidth(uint64 width)
{
    attributes.SetAttribute(Keys::EMBEDDED_WIDTH, width);
}

Optional<uint64> PipelineTextureNode::GetEmbeddedHeight() const
{
    return attributes.GetAttribute<uint64>(Keys::EMBEDDED_HEIGHT);
}

void PipelineTextureNode::SetEmbeddedHeight(uint64 height)
{
    attributes.SetAttribute(Keys::EMBEDDED_HEIGHT, height);
}
} // namespace se::editor
