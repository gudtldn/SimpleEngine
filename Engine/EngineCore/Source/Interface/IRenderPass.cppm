export module SimpleEngine.Interface.IRenderPass;

import SimpleEngine.Types;

import <SDL3/SDL_gpu.h>;


export
{
    class VirtualResourceHandle;
    // {
    // public:
    //     VirtualResourceHandle()
    //     {
    //         static uint32 next_resource_id = 0;
    //         resource_id = next_resource_id++;
    //     }
    //
    // private:
    //     uint32 resource_id;
    // };

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
    public:
        virtual ~IRenderPass() = default;

        // virtual void Setup(PassBuilder& builder) = 0;
        // virtual void Execute(SDL_GPUCommandBuffer* cmd, const ResolvedResources& resources) = 0;
    };
}
