#pragma once
#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineNode.h"
#include "SimpleEngine/Graphics/MaterialEnums.h"


namespace se::asset
{
class SE_CORE_API PipelineMaterialNode final : public PipelineNode<PipelineMaterialNode>
{
public:
    struct Keys
    {
        // Texture Slots
        inline static const StringName BASE_COLOR_TEX = "BaseColorTex";
        inline static const StringName NORMAL_TEX     = "NormalTex";
        inline static const StringName ROUGHNESS_TEX  = "RoughnessTex";
        inline static const StringName METALLIC_TEX   = "MetallicTex";
        inline static const StringName EMISSIVE_TEX   = "EmissiveTex";
        inline static const StringName OCCLUSION_TEX  = "OcclusionTex";
        inline static const StringName OPACITY_TEX    = "OpacityTex";

        // Scalar/Vector Parameters
        inline static const StringName BASE_COLOR_VAL = "BaseColorVal";
        inline static const StringName METALLIC_VAL   = "MetallicVal";
        inline static const StringName ROUGHNESS_VAL  = "RoughnessVal";
        inline static const StringName EMISSIVE_VAL   = "EmissiveVal";
        inline static const StringName OPACITY_VAL    = "OpacityVal";
        inline static const StringName SPECULAR_VAL   = "SpecularVal";

        // Settings
        inline static const StringName BLEND_MODE     = "BlendMode";
        inline static const StringName SHADING_MODEL  = "ShadingModel";
        inline static const StringName TWO_SIDED      = "TwoSided";
        inline static const StringName ALPHA_CUTOFF   = "AlphaCutoff";
    };

    virtual void GetFactoryDependencies(Array<Guid>& out_dependencies) const override;

public:
    // --- Textures ---
    [[nodiscard]] Optional<const Guid&> GetBaseColorTexture() const;
    void SetBaseColorTexture(const Guid& texture_uid);

    [[nodiscard]] Optional<const Guid&> GetNormalTexture() const;
    void SetNormalTexture(const Guid& texture_uid);

    [[nodiscard]] Optional<const Guid&> GetRoughnessTexture() const;
    void SetRoughnessTexture(const Guid& texture_uid);

    [[nodiscard]] Optional<const Guid&> GetMetallicTexture() const;
    void SetMetallicTexture(const Guid& texture_uid);

    [[nodiscard]] Optional<const Guid&> GetOcclusionTexture() const;
    void SetOcclusionTexture(const Guid& texture_uid);

    [[nodiscard]] Optional<const Guid&> GetOpacityTexture() const;
    void SetOpacityTexture(const Guid& texture_uid);

    // --- Values ---
    [[nodiscard]] Optional<const Vector4&> GetBaseColorValue() const;
    void SetBaseColorValue(const Vector4& color);

    [[nodiscard]] Optional<float> GetRoughnessValue() const;
    void SetRoughnessValue(float value);

    [[nodiscard]] Optional<float> GetMetallicValue() const;
    void SetMetallicValue(float value);

    [[nodiscard]] Optional<const Vector3&> GetEmissiveValue() const;
    void SetEmissiveValue(const Vector3& color);

    // --- Settings ---
    [[nodiscard]] graphics::EBlendMode GetBlendMode() const;
    void SetBlendMode(graphics::EBlendMode mode);

    [[nodiscard]] graphics::EShadingModel GetShadingModel() const;
    void SetShadingModel(graphics::EShadingModel model);

    [[nodiscard]] bool GetTwoSided() const;
    void SetTwoSided(bool use_two_sided);

    [[nodiscard]] float GetAlphaCutoff() const;
    void SetAlphaCutoff(float cutoff);

};
} // namespace se::asset
