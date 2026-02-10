#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineMaterialNode.h"


namespace se::asset
{
SE_BEGIN_REFLECT(PipelineMaterialNode)
SE_END_REFLECT(PipelineMaterialNode)

void PipelineMaterialNode::GetFactoryDependencies(Array<Guid>& out_dependencies) const
{
    PipelineBaseNode::GetFactoryDependencies(out_dependencies);

    // 의존성 검사 대상이 되는 텍스처 슬롯 키 목록
    static const FixedArray TextureSlots = {
        Keys::BASE_COLOR_TEX,
        Keys::NORMAL_TEX,
        Keys::ROUGHNESS_TEX,
        Keys::METALLIC_TEX,
        Keys::EMISSIVE_TEX,
        Keys::OCCLUSION_TEX,
        Keys::OPACITY_TEX,
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

Optional<const Guid&> PipelineMaterialNode::GetBaseColorTexture() const
{
    return attributes.GetAttribute<Guid>(Keys::BASE_COLOR_TEX);
}

void PipelineMaterialNode::SetBaseColorTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(Keys::BASE_COLOR_TEX, texture_uid);
}

Optional<const Guid&> PipelineMaterialNode::GetNormalTexture() const
{
    return attributes.GetAttribute<Guid>(Keys::NORMAL_TEX);
}

void PipelineMaterialNode::SetNormalTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(Keys::NORMAL_TEX, texture_uid);
}

Optional<const Guid&> PipelineMaterialNode::GetRoughnessTexture() const
{
    return attributes.GetAttribute<Guid>(Keys::ROUGHNESS_TEX);
}

void PipelineMaterialNode::SetRoughnessTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(Keys::ROUGHNESS_TEX, texture_uid);
}

Optional<const Guid&> PipelineMaterialNode::GetMetallicTexture() const
{
    return attributes.GetAttribute<Guid>(Keys::METALLIC_TEX);
}

void PipelineMaterialNode::SetMetallicTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(Keys::METALLIC_TEX, texture_uid);
}

Optional<const Guid&> PipelineMaterialNode::GetOcclusionTexture() const
{
    return attributes.GetAttribute<Guid>(Keys::OCCLUSION_TEX);
}

void PipelineMaterialNode::SetOcclusionTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(Keys::OCCLUSION_TEX, texture_uid);
}

Optional<const Guid&> PipelineMaterialNode::GetOpacityTexture() const
{
    return attributes.GetAttribute<Guid>(Keys::OPACITY_TEX);
}

void PipelineMaterialNode::SetOpacityTexture(const Guid& texture_uid)
{
    attributes.SetAttribute(Keys::OPACITY_TEX, texture_uid);
}

Optional<const Vector4&> PipelineMaterialNode::GetBaseColorValue() const
{
    return attributes.GetAttribute<Vector4>(Keys::BASE_COLOR_VAL);
}

void PipelineMaterialNode::SetBaseColorValue(const Vector4& color)
{
    attributes.SetAttribute(Keys::BASE_COLOR_VAL, color);
}

Optional<float> PipelineMaterialNode::GetRoughnessValue() const
{
    return attributes.GetAttribute<float>(Keys::ROUGHNESS_VAL);
}

void PipelineMaterialNode::SetRoughnessValue(float value)
{
    attributes.SetAttribute(Keys::ROUGHNESS_VAL, value);
}

Optional<float> PipelineMaterialNode::GetMetallicValue() const
{
    return attributes.GetAttribute<float>(Keys::METALLIC_VAL);
}

void PipelineMaterialNode::SetMetallicValue(float value)
{
    attributes.SetAttribute(Keys::METALLIC_VAL, value);
}

Optional<const Vector3&> PipelineMaterialNode::GetEmissiveValue() const
{
    return attributes.GetAttribute<Vector3>(Keys::EMISSIVE_VAL);
}

void PipelineMaterialNode::SetEmissiveValue(const Vector3& color)
{
    attributes.SetAttribute(Keys::EMISSIVE_VAL, color);
}

graphics::EBlendMode PipelineMaterialNode::GetBlendMode() const
{
    return attributes.GetAttribute<graphics::EBlendMode>(Keys::BLEND_MODE).ValueOr(graphics::EBlendMode::Opaque);
}

void PipelineMaterialNode::SetBlendMode(graphics::EBlendMode mode)
{
    attributes.SetAttribute(Keys::BLEND_MODE, static_cast<uint64>(mode));
}

graphics::EShadingModel PipelineMaterialNode::GetShadingModel() const
{
    return attributes.GetAttribute<graphics::EShadingModel>(Keys::SHADING_MODEL).ValueOr(graphics::EShadingModel::Lit);
}

void PipelineMaterialNode::SetShadingModel(graphics::EShadingModel model)
{
    attributes.SetAttribute(Keys::SHADING_MODEL, static_cast<uint64>(model));
}

bool PipelineMaterialNode::GetTwoSided() const
{
    return attributes.GetAttribute<bool>(Keys::TWO_SIDED).ValueOr(false);
}

void PipelineMaterialNode::SetTwoSided(bool use_two_sided)
{
    attributes.SetAttribute(Keys::TWO_SIDED, use_two_sided);
}

float PipelineMaterialNode::GetAlphaCutoff() const
{
    return attributes.GetAttribute<float>(Keys::ALPHA_CUTOFF).ValueOr(0.5f);
}

void PipelineMaterialNode::SetAlphaCutoff(float cutoff)
{
    attributes.SetAttribute(Keys::ALPHA_CUTOFF, cutoff);
}
}  // namespace se::asset
