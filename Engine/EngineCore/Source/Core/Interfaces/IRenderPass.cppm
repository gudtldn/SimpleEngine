export module SimpleEngine.Interfaces.IRenderPass;

import SimpleEngine.Platform.Types;


export
{
    enum class ERenderPassType : uint8
    {
        Unknown = 0,
        ShadowMap,
        GBuffer,
        Lighting,
        Forward,
        PostProcess,
        UI
    };

    class IRenderPass
    {
        virtual ~IRenderPass() = default;

        virtual ERenderPassType GetType() const = 0;
    };
}
