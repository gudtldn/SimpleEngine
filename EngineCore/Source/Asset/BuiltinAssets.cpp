#include "SimpleEngine/Asset/BuiltinAssets.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"

#include <memory>


namespace se
{
void SeedBuiltinAssets(AssetSubsystem& subsystem)
{
    // DefaultLit Material (템플릿) + DefaultLitInstance (흰색 기본값)
    {
        auto mat = std::make_unique<Material>();
        mat->vertex_shader = "CoreShader://Default.vert";
        mat->fragment_shader = "CoreShader://Default.frag";
        mat->blend_mode = EBlendMode::Opaque;
        mat->shading_model = EShadingModel::Lit;

        mat->texture_slots.Push({
            .name = "BaseColor",
            .fragment_slot = 0,
            .sampler = ESamplerType::LinearRepeat,
            .default_texture_id = BuiltinAssetIds::White1x1,
        });

        // std140 자동 패킹: base_color(16B) + alpha_cutoff(4B) + flags(4B) + pad(8B) = 32B
        mat->AddParameter("base_color",   EMaterialParamType::Float4, { 1.0f, 1.0f, 1.0f, 1.0f });
        mat->AddParameter("alpha_cutoff", EMaterialParamType::Float,  { 0.5f, 0.0f, 0.0f, 0.0f });
        mat->AddParameter("flags",        EMaterialParamType::Uint,   { 0.0f, 0.0f, 0.0f, 0.0f });
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

    // White1x1 Texture2D (1x1 흰 픽셀, sRGB, GPU 업로드는 EnsureTexturesResident에서 수행)
    {
        auto tex = std::make_unique<Texture2D>();
        tex->width = 1;
        tex->height = 1;
        tex->format = ETextureFormat::R8G8B8A8_UNORM_SRGB;
        tex->generate_mips = false;
        tex->pixels.Push(255);
        tex->pixels.Push(255);
        tex->pixels.Push(255);
        tex->pixels.Push(255);

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::White1x1, std::move(tex)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register White1x1 Texture2D.");
        }
    }

    ConsoleLog(ELogLevel::Info, "SeedBuiltinAssets: DefaultLit, DefaultLitInstance, White1x1 registered.");
}
} // namespace se
