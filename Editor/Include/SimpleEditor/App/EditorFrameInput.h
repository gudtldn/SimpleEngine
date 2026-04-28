#pragma once

#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEditor/UI/ViewModeTypes.h"

#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/Graphics/View/RenderView.h"

#include "SDL3/SDL_gpu.h"


namespace se::editor
{
/**
 * 뷰포트 렌더에 필요한 데이터 스냅샷
 */
struct ViewportRenderInput
{
    RenderView render_view;
    StringName color_target_name;
    StringName depth_target_name;
    EViewMode view_mode = EViewMode::Perspective;

    // Entity pick 처리
    bool need_entity_pick = false;
    Vector2f cursor_viewport_pos;

    // raw GPU 텍스처
    SDL_GPUTexture* color_texture_raw = nullptr;
    SDL_GPUTexture* entity_id_texture = nullptr;
    SDL_GPUTexture* gizmo_pick_texture = nullptr;

    // Gizmo (non-owning pointer)
    GizmoDrawList* gizmo_draw_list = nullptr;
    bool show_gizmo_pick_pass = false;
};
} // namespace se::editor
