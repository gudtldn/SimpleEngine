#include "SimpleEngine/Asset/BuiltinAssets.h"

#include "SimpleEngine/Asset/AssetSubsystem.h"
#include "SimpleEngine/Asset/Types/Material.h"
#include "SimpleEngine/Asset/Types/MaterialInstance.h"
#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Graphics/Material/MaterialParameterDescriptor.h"
#include "SimpleEngine/Graphics/Material/MaterialTextureSlot.h"

#include <cstring>
#include <memory>


namespace se::asset
{
void SeedBuiltinAssets(AssetSubsystem& subsystem)
{
    // DefaultLit Material (템플릿)
    {
        auto mat = std::make_unique<Material>();
        mat->vertex_shader = "CoreShader://Default.vert";
        mat->fragment_shader = "CoreShader://Default.frag";
        mat->blend_mode = graphics::EBlendMode::Opaque;
        mat->shading_model = graphics::EShadingModel::Lit;

        mat->texture_slots.Push({
            .name = "BaseColor",
            .fragment_slot = 0,
            .sampler = graphics::ESamplerType::LinearRepeat,
            .default_texture_id = BuiltinAssetIds::White1x1,
        });
        mat->parameter_layout.Push({
            .name = "base_color",
            .type = graphics::EMaterialParamType::Float4,
            .offset = 0,
            .default_value = { 1.0f, 1.0f, 1.0f, 1.0f },
        });
        mat->parameter_layout.Push({
            .name = "alpha_cutoff",
            .type = graphics::EMaterialParamType::Float,
            .offset = 16,
            .default_value = { 0.5f, 0.0f, 0.0f, 0.0f },
        });

        if (!subsystem.RegisterBuiltin(BuiltinAssetIds::DefaultLit, std::move(mat)).IsValid())
        {
            ConsoleLog(ELogLevel::Error, "SeedBuiltinAssets: Failed to register DefaultLit Material.");
        }
    }

    // DefaultLitInstance MaterialInstance (흰색, parent = DefaultLit)
    {
        auto inst = std::make_unique<MaterialInstance>();
        inst->parent_material_id = BuiltinAssetIds::DefaultLit;

        // DefaultLitMaterialUBO: base_color(16B) + alpha_cutoff(4B) + flags(4B) + _pad(8B) = 32B
        struct alignas(16) DefaultLitMaterialUBO
        {
            LinearColor base_color = LinearColor::White();
            float alpha_cutoff = 0.5f;
            uint32 flags = 0;
        };
        static_assert(sizeof(DefaultLitMaterialUBO) == 32);

        constexpr DefaultLitMaterialUBO ubo{};
        inst->parameter_values.ResizeUninitialized(sizeof(ubo));
        std::memcpy(inst->parameter_values.Data(), &ubo, sizeof(ubo));

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
        tex->channels = 4;
        tex->format = ETextureFormat::R8G8B8A8;
        tex->is_srgb = true;
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
} // namespace se::asset
