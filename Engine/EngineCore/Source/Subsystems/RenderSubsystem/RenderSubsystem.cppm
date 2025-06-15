export module SimpleEngine.Subsystems.RenderSubsystem;

import SimpleEngine.Core.ISubsystem;
import std;
import <SDL3/SDL.h>;


export class RenderSubsystem : public ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

    virtual std::vector<::std::type_index> GetDependencies() const override;

public:
    SDL_GPUDevice* GetGpuDevice() const { return gpu_device; }

private:
    SDL_GPUDevice* gpu_device = nullptr;
    SDL_Window* cached_window = nullptr;
};
