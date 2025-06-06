export module SimpleEngine.Editor.App;
import SimpleEngine.App;
import <SDL3/SDL.h>;


export class EditorApplication : public Application
{
public:
    EditorApplication();

protected:
    virtual bool InitializeSubSystems() override;
    virtual bool PostInitialize() override;
    virtual void PreRelease() override;
    virtual void Update(float delta_time) override;

    virtual void PreRender() override;
    virtual void Render() override;
    virtual void PostRender() override;

private:
    SDL_Window* cached_window = nullptr;
    SDL_GPUDevice* cached_gpu_device = nullptr;
};
