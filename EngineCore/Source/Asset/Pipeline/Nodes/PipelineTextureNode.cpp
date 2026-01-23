#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineTextureNode.h"


namespace se::asset
{
refl::TypeId PipelineTextureNode::GetTypeId() const noexcept
{
    return refl::TypeId::Get<PipelineTextureNode>();
}

Optional<const String&> PipelineTextureNode::GetSourceFile() const
{
    return attributes.GetAttribute<String>(Keys::SOURCE_FILE);
}

void PipelineTextureNode::SetSourceFile(const String& file_path)
{
    attributes.SetAttribute(Keys::SOURCE_FILE, file_path);
}

bool PipelineTextureNode::IsSRGB() const
{
    // TODO: 기본값 true/false 정책 결정 필요 (보통 Albedo는 true, Normal/Mask는 false)
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
}  // namespace se::asset
