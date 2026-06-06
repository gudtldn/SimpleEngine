#pragma once

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/Material/SamplerType.h"


namespace se
{
/**
 * 머티리얼 텍스처 슬롯 정의
 */
struct SE_ANNOTATION(=meta::Reflect) MaterialTextureSlot
{
    // 슬롯 식별 이름 (예: "BaseColor", "Normal")
    SE_ANNOTATION(=meta::Reflect)
    StringName name;

    // SDL_BindGPUFragmentSamplers에 전달할 슬롯 번호
    SE_ANNOTATION(=meta::Reflect)
    u32 fragment_slot = 0;

    // 이 슬롯에 사용할 샘플러 종류
    SE_ANNOTATION(=meta::Reflect)
    ESamplerType sampler = ESamplerType::LinearRepeat;

    // 인스턴스가 오버라이드하지 않을 때 사용할 폴백 텍스처 AssetId
    SE_ANNOTATION(=meta::Reflect)
    AssetId default_texture_id;
};
} // namespace se

SE_DECLARE_REFLECTION(se::MaterialTextureSlot)
