#pragma once

#include "SimpleEngine/Core/Interfaces/ISubsystem.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"
#include "SimpleEngine/Reflection/SubsystemRegistration.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor::ui
{
class ViewportPanel;


/**
 * Viewport Render Data
 */
struct ViewportRenderInfo
{
    SDL_GPUTexture* color_texture = nullptr;
    uint32 width = 0;
    uint32 height = 0;
    // TODO: 뷰포트별 카메라, 씬 정보 등 추가
};

/**
 * @todo docs
 */
class EditorViewportSubsystem : public core::ISubsystem<RenderSubsystem>
{
    SE_REGISTER_SUBSYSTEM(EditorViewportSubsystem)

public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

public:
    /** TODO: docs */
    [[nodiscard]] SDL_GPUTexture* UpdateAndGetViewportTexture(const StringName& viewport_id, uint32 new_width, uint32 new_height);

    /** TODO: docs */
    [[nodiscard]] const auto& GetActiveViewportInfo() const { return viewport_data; }

private:
    SDL_GPUDevice* gpu_device = nullptr;
    HashMap<StringName, ViewportRenderInfo> viewport_data;
};
}
