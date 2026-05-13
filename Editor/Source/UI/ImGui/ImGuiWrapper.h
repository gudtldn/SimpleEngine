#pragma once

#include "imgui.h"


namespace ImGui
{
bool DragScalarNInfinity(
    const char* label, ImGuiDataType data_type, void* p_data, int components,
    f32 v_speed = 1.0f, const void* p_min = nullptr, const void* p_max = nullptr, const char* format = nullptr,
    ImGuiSliderFlags flags = 0
);
} // namespace ImGui
