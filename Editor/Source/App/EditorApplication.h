#pragma once
#include "SimpleEngine/App/Application.h"

#include "SDL3/SDL.h"


namespace se::editor
{
class EditorApplication : public se::app::Application
{
public:
    EditorApplication();

protected:
    virtual void RegisterSubsystems() override;
    virtual bool PostInitialize() override;
    // virtual void PreRelease() override;
    // virtual void Update(float delta_time) override;

    virtual void Render() override;

private:
    SDL_Window* cached_window = nullptr;
    SDL_GPUDevice* cached_gpu_device = nullptr;
};
}
