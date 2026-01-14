#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineMaterialNode.h"


namespace se::asset
{
// Texture Slots (Dependencies)
StringName PipelineMaterialNode::AttributeKeys::BaseColorTex = "BaseColorTex";
StringName PipelineMaterialNode::AttributeKeys::NormalTex = "NormalTex";
StringName PipelineMaterialNode::AttributeKeys::RoughnessTex = "RoughnessTex";
StringName PipelineMaterialNode::AttributeKeys::MetallicTex = "MetallicTex";
StringName PipelineMaterialNode::AttributeKeys::EmissiveTex = "EmissiveTex";

// Scalar/Vector Parameters
StringName PipelineMaterialNode::AttributeKeys::BaseColorVal = "BaseColorVal";
StringName PipelineMaterialNode::AttributeKeys::MetallicVal = "MetallicVal";
StringName PipelineMaterialNode::AttributeKeys::RoughnessVal = "RoughnessVal";

// Settings
StringName PipelineMaterialNode::AttributeKeys::BlendMode = "BlendMode";       // Opaque, Masked, Translucent
StringName PipelineMaterialNode::AttributeKeys::ShadingModel = "ShadingModel"; // DefaultLit, Unlit
StringName PipelineMaterialNode::AttributeKeys::TwoSided = "TwoSided";

refl::TypeId PipelineMaterialNode::GetTypeId() const noexcept
{
    return refl::TypeId::Get<PipelineMaterialNode>();
}

void PipelineMaterialNode::GetFactoryDependencies(Array<Guid>& out_dependencies) const
{
    PipelineBaseNode::GetFactoryDependencies(out_dependencies);

    // 의존성 검사 대상이 되는 텍스처 슬롯 키 목록
    static const std::initializer_list TextureSlots = {
        AttributeKeys::BaseColorTex,
        AttributeKeys::NormalTex,
        AttributeKeys::RoughnessTex,
        AttributeKeys::MetallicTex,
        AttributeKeys::EmissiveTex
    };

    for (const StringName& key : TextureSlots)
    {
        // 해당 키에 Guid 타입의 값이 설정되어 있다면 의존성에 추가
        if (Optional uid = attributes.GetAttribute<Guid>(key))
        {
            if (uid->IsValid())
            {
                out_dependencies.Push(*uid);
            }
        }
    }
}

Optional<Guid> PipelineMaterialNode::GetBaseColorTexture() const
{
    return attributes.GetAttribute<Guid>(AttributeKeys::BaseColorTex).Copy();
}

void PipelineMaterialNode::SetBaseColorTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(AttributeKeys::BaseColorTex, texture_uid);
}

void PipelineMaterialNode::SetNormalTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(AttributeKeys::NormalTex, texture_uid);
}

void PipelineMaterialNode::SetRoughnessTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(AttributeKeys::RoughnessTex, texture_uid);
}

void PipelineMaterialNode::SetBaseColorValue(const Vector4& color)
{
    attributes.SetAttribute(AttributeKeys::BaseColorVal, color);
}

void PipelineMaterialNode::SetRoughnessValue(float value)
{
    attributes.SetAttribute(AttributeKeys::RoughnessVal, value);
}

void PipelineMaterialNode::SetBlendMode(int32 mode)
{
    attributes.SetAttribute(AttributeKeys::BlendMode, static_cast<int64>(mode));
}

void PipelineMaterialNode::SetTwoSided(bool bEnable)
{
    attributes.SetAttribute(AttributeKeys::TwoSided, bEnable);
}
} // namespace se::asset
