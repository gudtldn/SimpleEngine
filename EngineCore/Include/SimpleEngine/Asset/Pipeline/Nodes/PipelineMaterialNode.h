#pragma once
#include "SimpleEngine/Asset/Pipeline/Nodes/PipelineNodeBase.h"


namespace se::asset
{
class PipelineMaterialNode final : public PipelineBaseNode
{
public:
    struct AttributeKeys
    {
        // Texture Slots (Dependencies)
        static StringName BaseColorTex;
        static StringName NormalTex;
        static StringName RoughnessTex;
        static StringName MetallicTex;
        static StringName EmissiveTex;

        // Scalar/Vector Parameters
        static StringName BaseColorVal;
        static StringName MetallicVal;
        static StringName RoughnessVal;

        // Settings
        static StringName BlendMode;    // Opaque, Masked, Translucent
        static StringName ShadingModel; // DefaultLit, Unlit
        static StringName TwoSided;
    };

    [[nodiscard]] virtual refl::TypeId GetTypeId() const noexcept override;
    virtual void GetFactoryDependencies(Array<Guid>& out_dependencies) const override;

public:
    [[nodiscard]] Optional<Guid> GetBaseColorTexture() const;
    void SetBaseColorTexture(const Guid& texture_uid);

    void SetNormalTexture(const Guid& texture_uid);
    void SetRoughnessTexture(const Guid& texture_uid);

    void SetBaseColorValue(const Vector4& color);
    void SetRoughnessValue(float value);

    void SetBlendMode(int32 mode);
    void SetTwoSided(bool bEnable);
};
} // namespace se::asset
