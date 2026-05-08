#pragma once

#include "SimpleEngine/Core/Container/String.h"

#include "imgui.h"


namespace ImGui
{
// se::String을 위한 InputText 오버로딩
bool InputText(
    const char* label, se::String* str,
    ImGuiInputTextFlags flags = 0,
    ImGuiInputTextCallback callback = nullptr,
    void* user_data = nullptr
);
bool InputTextMultiline(
    const char* label, se::String* str,
    const ImVec2& size = ImVec2(0, 0),
    ImGuiInputTextFlags flags = 0,
    ImGuiInputTextCallback callback = nullptr,
    void* user_data = nullptr
);
bool InputTextWithHint(
    const char* label, const char* hint, se::String* str,
    ImGuiInputTextFlags flags = 0,
    ImGuiInputTextCallback callback = nullptr,
    void* user_data = nullptr
);
} // namespace ImGui
