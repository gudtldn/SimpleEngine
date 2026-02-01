#pragma once

#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Subsystem/ISubsystem.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Gfx/RenderSubsystem.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
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

    Matrix4x4 view_matrix = Matrix4x4::Identity();
    Matrix4x4 projection_matrix = Matrix4x4::Identity();
    // TODO: 뷰포트별 카메라, 씬 정보 등 추가
};

/**
 * @todo docs
 */
class EditorViewportSubsystem : public ISubsystem
{
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
}  // namespace se::editor
