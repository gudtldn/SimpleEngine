#include "SimpleEditor/Asset/Pipeline/Factories/MaterialInstanceFactory.h"
#include "SimpleEditor/Asset/Pipeline/Nodes/PipelineMaterialInstanceNode.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/BuiltinAssets.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Cast.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"

#include "tracy/Tracy.hpp"


namespace se::editor
{
TypeId MaterialInstanceFactory::GetAssetType() const
{
    return TypeId::Get<MaterialInstance>();
}

bool MaterialInstanceFactory::CanCreateAsset(const PipelineBaseNode* node) const
{
    return IsA<PipelineMaterialInstanceNode>(node);
}

std::shared_ptr<AssetBase> MaterialInstanceFactory::CreateAsset(
    PipelineBaseNode* node,
    const PipelineImportContext& context
)
{
    ZoneScopedN("MaterialInstanceFactory::CreateAsset");
    std::ignore = context;

    const PipelineMaterialInstanceNode* mat_node = CastChecked<const PipelineMaterialInstanceNode>(node);

    // DefaultLit 머티리얼 로드 (빌트인이므로 항상 메모리에 상주)
    const AssetSubsystem* asset_sub = GetSubsystem<AssetSubsystem>();
    if (!asset_sub)
    {
        ConsoleLog(ELogLevel::Error, "MaterialInstanceFactory: AssetSubsystem not available.");
        return nullptr;
    }

    const AssetHandle<Material> parent_handle = asset_sub->Find<Material>(BuiltinAssetIds::DefaultLit);
    if (!parent_handle.IsValid())
    {
        ConsoleLog(ELogLevel::Error, "MaterialInstanceFactory: DefaultLit material not found.");
        return nullptr;
    }
    const Material& parent = *parent_handle;

    auto inst = std::make_shared<MaterialInstance>();
    inst->parent_material_id = BuiltinAssetIds::DefaultLit;
    inst->InitializeFromParent(parent);

    // 파라미터 오버라이드 적용
    for (const auto& [name, value] : mat_node->param_overrides)
    {
        if (const auto param = parent.FindParameter(name))
        {
            const uint32 size = param->GetSize();
            if (param->offset + size <= static_cast<uint32>(inst->parameter_values.Len()))
            {
                std::memcpy(inst->parameter_values.Data() + param->offset, &value, size);
            }
        }
    }

    // 텍스처 슬롯 바인딩
    for (const auto& [slot_name, tex_uid] : mat_node->texture_node_refs)
    {
        // 노드 UID == AssetId.guid 임을 이용해 노드 UID를 바로 사용
        if (tex_uid.IsValid())
        {
            inst->texture_overrides.Insert(slot_name, AssetId{ tex_uid });
        }
    }

    return inst;
}
} // namespace se::editor
