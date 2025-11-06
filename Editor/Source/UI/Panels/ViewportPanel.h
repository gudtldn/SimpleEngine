#pragma once
#include "UI/Panels/IEditorPanel.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "SDL3/SDL_gpu.h"
#include "SimpleEngine/Core/Types/StringName.h"


namespace se::editor::ui
{
class ViewportPanel : public IEditorPanel
{
public:
    virtual ~ViewportPanel() override;

    [[nodiscard]] virtual const char* GetName() const override;

public:
    [[nodiscard]] SDL_GPUTexture* GetViewportColorTexture() const { return viewport_color_texture; }
    [[nodiscard]] SDL_GPUTexture* GetViewportDepthTexture() const { return viewport_depth_texture; }

    [[nodiscard]] uint32 GetViewportWidth() const { return viewport_width; }
    [[nodiscard]] uint32 GetViewportHeight() const { return viewport_height; }
    virtual void Draw() override;

private:
    void ResizeViewportTexture(uint32 new_width, uint32 new_height);

private:
    SDL_GPUTexture* viewport_color_texture = nullptr;
    SDL_GPUTexture* viewport_depth_texture = nullptr;
    uint32 viewport_width = 0;
    uint32 viewport_height = 0;
};
}
