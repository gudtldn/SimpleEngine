#include "SimpleEngine/Asset/BuiltinAssets.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"

#include <memory>


namespace se
{
void SeedBuiltinAssets(AssetSubsystem& subsystem)
{
    // -------------------------------------------------------------------------
    // 1. DefaultLit Material & Instance
    // glTF PBR Metallic-Roughness 표준을 따르는 기본 머티리얼 및 인스턴스
    // -------------------------------------------------------------------------
    {
        auto mat = std::make_unique<Material>();
        mat->vertex_shader = "CoreShader://Default.vert";
        mat->fragment_shader = "CoreShader://Default.frag";
        mat->blend_mode = EBlendMode::Opaque;
        mat->shading_model = EShadingModel::Lit;

        // 텍스처 슬롯 정의 (fragment shader binding 순서와 일치)
        mat->texture_slots.Push({ .name = "BaseColor",         .fragment_slot = 0, .sampler = ESamplerType::LinearRepeat, .default_texture_id = BuiltinAssetIds::White1x1   });
        mat->texture_slots.Push({ .name = "MetallicRoughness", .fragment_slot = 1, .sampler = ESamplerType::LinearRepeat, .default_texture_id = BuiltinAssetIds::DefaultORM });
        mat->texture_slots.Push({ .name = "Normal",            .fragment_slot = 2, .sampler = ESamplerType::LinearRepeat, .default_texture_id = BuiltinAssetIds::FlatNormal });
        mat->texture_slots.Push({ .name = "Occlusion",         .fragment_slot = 3, .sampler = ESamplerType::LinearRepeat, .default_texture_id = BuiltinAssetIds::DefaultORM });
        mat->texture_slots.Push({ .name = "Emissive",          .fragment_slot = 4, .sampler = ESamplerType::LinearRepeat, .default_texture_id = BuiltinAssetIds::Black1x1   });

        // 파라미터 정의: std140 UBO 레이아웃 (총 48 bytes):
        //   [0:15]  base_color_factor (Float4) 베이스 컬러 팩터
        //   [16:19] metallic_factor   (Float)  메탈릭 팩터 (0.0=비금속, 1.0=금속)
        //   [20:23] roughness_factor  (Float)  러프니스 팩터 (0.0=매끄러움, 1.0=거침)
        //   [24:27] alpha_cutoff      (Float)  마스크 블렌드 모드에서 알파 임계값
        //   [28:31] flags             (Uint)   렌더링 상태 비트 플래그 (추후 has_normal_map 등 확장)
        //   [32:47] emissive_factor   (Float4) 이미시브 팩터 (xyz 사용, w=0 예약)
        mat->AddParameter("base_color_factor", EMaterialParamType::Float4, { 1.0f, 1.0f, 1.0f, 1.0f });
        mat->AddParameter("metallic_factor",   EMaterialParamType::Float,  { 0.0f, 0.0f, 0.0f, 0.0f });
        mat->AddParameter("roughness_factor",  EMaterialParamType::Float,  { 0.5f, 0.0f, 0.0f, 0.0f });
        mat->AddParameter("alpha_cutoff",      EMaterialParamType::Float,  { 0.5f, 0.0f, 0.0f, 0.0f });
        mat->AddParameter("flags",             EMaterialParamType::Uint,   { 0.0f, 0.0f, 0.0f, 0.0f });
        mat->AddParameter("emissive_factor",   EMaterialParamType::Float4, { 0.0f, 0.0f, 0.0f, 0.0f });
        mat->FinalizeLayout();

        auto inst = std::make_unique<MaterialInstance>();
        inst->parent_material_id = BuiltinAssetIds::DefaultLit;
        inst->InitializeFromParent(*mat);

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::DefaultLit, std::move(mat)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register DefaultLit Material.");
        }

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::DefaultLitInstance, std::move(inst)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register DefaultLitInstance.");
        }
    }

    // -------------------------------------------------------------------------
    // 2. Built-in Textures
    // -------------------------------------------------------------------------

    // White1x1: 1x1 흰색 픽셀 (sRGB)
    // 용도: BaseColor등 텍스처가 누락되었을 때 팩터 값을 온전히 보존하기 위한 기본값
    {
        auto tex = std::make_unique<Texture2D>();
        tex->width = 1;
        tex->height = 1;
        tex->format = ETextureFormat::R8G8B8A8_UNORM_SRGB;
        tex->generate_mips = false;
        tex->pixels = { 255, 255, 255, 255 };

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::White1x1, std::move(tex)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register White1x1 Texture2D.");
        }
    }

    // Black1x1: 1x1 검은색 픽셀 (sRGB)
    // 용도: Emissive 텍스처 기본값
    {
        auto tex = std::make_unique<Texture2D>();
        tex->width = 1;
        tex->height = 1;
        tex->format = ETextureFormat::R8G8B8A8_UNORM_SRGB;
        tex->generate_mips = false;
        tex->pixels = { 0, 0, 0, 255 };

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::Black1x1, std::move(tex)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register Black1x1 Texture2D.");
        }
    }

    // FlatNormal: 1x1 접선 공간(Tangent Space) 플랫 노멀 (Linear)
    // 용도: 셰이더에서 디코딩 시 Z축(0, 0, 1) 평면 벡터가 되도록 인코딩된 값. 굴곡 없는 완벽한 평면을 표현.
    {
        auto tex = std::make_unique<Texture2D>();
        tex->width = 1;
        tex->height = 1;
        tex->format = ETextureFormat::R8G8B8A8_UNORM;
        tex->generate_mips = false;
        tex->pixels = { 128, 128, 255, 255 };

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::FlatNormal, std::move(tex)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register FlatNormal Texture2D.");
        }
    }

    // DefaultORM: 1x1 ORM (Occlusion, Roughness, Metallic) 데이터 텍스처 (Linear)
    // 용도: 물리 연산용 데이터
    {
        auto tex = std::make_unique<Texture2D>();
        tex->width = 1;
        tex->height = 1;
        tex->format = ETextureFormat::R8G8B8A8_UNORM;
        tex->generate_mips = false;

        // R (Occlusion) = 255 (1.0, 차폐 없음, 완전히 밝음)
        // G (Roughness) = 128 (0.5, 적당히 매끄러운 표면 반사)
        // B (Metallic)  = 0   (0.0, 비금속/Dielectric)
        // A (Unused)    = 255
        tex->pixels = { 255, 128, 0, 255 };

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::DefaultORM, std::move(tex)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register DefaultORM Texture2D.");
        }
    }

    ConsoleLog(ELogLevel::Info, "SeedBuiltinAssets registered.");
}
} // namespace se
