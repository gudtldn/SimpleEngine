#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineTextureNode.h"


namespace se::asset
{
StringName PipelineTextureNode::AttributeKeys::SourceFile = "SourceFile";
StringName PipelineTextureNode::AttributeKeys::IsSRGB = "IsSRGB";
StringName PipelineTextureNode::AttributeKeys::Compression = "Compression";
StringName PipelineTextureNode::AttributeKeys::Filter = "Filter";
StringName PipelineTextureNode::AttributeKeys::LODGroup = "LODGroup";

refl::TypeId PipelineTextureNode::GetTypeId() const noexcept
{
    return refl::TypeId::Get<PipelineTextureNode>();
}

void PipelineTextureNode::SetSourceFile(const String& path)
{
    attributes.SetAttribute(AttributeKeys::SourceFile, path);
}

Optional<const String&> PipelineTextureNode::GetSourceFile() const
{
    return attributes.GetAttribute<String>(AttributeKeys::SourceFile);
}

void PipelineTextureNode::SetSRGB(bool is_srgb)
{
    attributes.SetAttribute(AttributeKeys::IsSRGB, is_srgb);
}

bool PipelineTextureNode::GetSRGB() const
{
    return attributes.GetAttribute<bool>(AttributeKeys::IsSRGB).ValueOr(true);
}

void PipelineTextureNode::SetCompression(const String& compression)
{
    attributes.SetAttribute(AttributeKeys::Compression, compression);
}
}  // namespace se::asset
