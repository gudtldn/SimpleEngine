#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Graphics/Scene/SceneDrawData.h"
#include "SimpleEngine/Graphics/View/RenderView.h"


namespace se::graphics
{
/**
 * 한 프레임의 렌더링에 필요한 모든 데이터를 묶는 최상위 구조체
 * @note Application 레이어에서 생성되어, RenderSubsystem에 전달됩니다.
 */
struct FramePacket
{
    Array<RenderView> render_views;
    SceneDrawData scene_draw_data;
    uint64 frame_number = 0;
};
} // namespace se::graphics
