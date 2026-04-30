// NOLINTBEGIN(*-reserved-identifier)
#include "SimpleEditor/Asset/Pipeline/Factories/MaterialFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineMaterialNode.h"

#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"

#include "tracy/Tracy.hpp"

#include <cstring>
#include <memory>


namespace se::editor
{
TypeId MaterialFactory::GetAssetType() const
{
    return TypeId::Get<asset::MaterialInstance>();
}

bool MaterialFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<PipelineMaterialNode>(node);
}

std::shared_ptr<asset::AssetBase> MaterialFactory::CreateAsset(PipelineBaseNode* in_node, const PipelineImportContext& context)
{
    ZoneScopedN("MaterialFactory::CreateAsset");

    const PipelineMaterialNode* node = CastChecked<const PipelineMaterialNode>(in_node);

    auto instance = std::make_shared<asset::MaterialInstance>();
    instance->parent_material_id = asset::BuiltinAssetIds::DefaultLit;

    // DefaultLitMaterialUBO: base_color(16B) + alpha_cutoff(4B) + flags(4B) + _pad(8B) = 32B
    // BuiltinAssets.cpp의 DefaultLitMaterialUBO와 레이아웃을 동일하게 유지합니다.
    struct alignas(16) DefaultLitMaterialUBO
    {
        float base_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // offset 0
        float alpha_cutoff  = 0.5f;                          // offset 16
        uint32 flags        = 0;                             // offset 20
        // implicit padding 8B
    };
    static_assert(sizeof(DefaultLitMaterialUBO) == 32);

    DefaultLitMaterialUBO ubo{};

    // base_color: 텍스처가 없을 때 노드의 값, 있으면 기본 흰색(tint)
    if (const Optional<const Vector4&> color = node->GetBaseColorValue())
    {
        ubo.base_color[0] = color->x;
        ubo.base_color[1] = color->y;
        ubo.base_color[2] = color->z;
        ubo.base_color[3] = color->w;
    }

    // alpha_cutoff: Masked 블렌드 모드일 때만 노드 값 사용
    if (node->GetBlendMode() == graphics::EBlendMode::Masked)
    {
        ubo.alpha_cutoff = node->GetAlphaCutoff();
    }

    instance->parameter_values.ResizeUninitialized(sizeof(ubo));
    std::memcpy(instance->parameter_values.Data(), &ubo, sizeof(ubo));

    // BaseColor 텍스처 오버라이드
    if (const Optional<const Guid&> tex_uid = node->GetBaseColorTexture())
    {
        if (const Optional<asset::AssetId> tex_id = context.GetCreatedAssetId(*tex_uid))
        {
            instance->texture_overrides.Insert("BaseColor", *tex_id);
        }
    }

    // Emissive 텍스처 오버라이드
    if (const Optional<const Guid&> tex_uid = node->GetEmissiveTexture())
    {
        if (const Optional<asset::AssetId> tex_id = context.GetCreatedAssetId(*tex_uid))
        {
            instance->texture_overrides.Insert("Emissive", *tex_id);
        }
    }

    return instance;
}
} // namespace se::editor
// NOLINTEND(*-reserved-identifier)
