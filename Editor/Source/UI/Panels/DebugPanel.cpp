#include "UI/Panels/DebugPanel.h"

#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Engine/Engine.h"
#include "SimpleEngine/Core/Input/InputSubsystem.h"
#include "SimpleEngine/Core/Memory/MemoryStats.h"

#include "imgui.h"


namespace
{
se::String FormatBytes(usize bytes)
{
    if (bytes < 1024)
    {
        return se::String::Format("{} B", bytes);
    }
    if (bytes < 1024ULL * 1024)
    {
        return se::String::Format("{:.2f} KB", static_cast<double>(bytes) / 1024.0);
    }
    if (bytes < 1024ULL * 1024 * 1024)
    {
        return se::String::Format("{:.2f} MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    }
    return se::String::Format("{:.2f} GB", static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0));
}
}  // namespace

namespace se::editor
{
DebugPanel::DebugPanel()
{
    SetVisibility(false);
}

const char* DebugPanel::GetName() const
{
    return "Debug";
}

ImGuiWindowFlags DebugPanel::GetWindowFlags() const
{
    return ImGuiWindowFlags_MenuBar;
}

void DebugPanel::DrawContent()
{
    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("Views"))
    {
        SE_SCOPE_DEFER{ ImGui::EndMenu(); };
    }
    ImGui::EndMenuBar();

    if (ImGui::CollapsingHeader("Frame Counter", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static constexpr int32 max_samples = 200;
        static Array fps_history(max_samples, 0.0f);
        static int32 offset = 0;

        const float dt = static_cast<float>(Engine::GetDeltaTime());
        const float fps = 1.0f / dt;

        fps_history[offset] = fps;
        offset = (offset + 1) % max_samples;

        const String overlay = String::Format("FPS: {:.2f}, DT: {:.6f}s", fps, dt);
        ImGui::PlotLines(
            "##FPS_Graph",
            fps_history.Data(),
            static_cast<int>(fps_history.Len()),
            offset,
            overlay.CStr(),
            0.0f,
            FLT_MAX,
            ImVec2(0, 80)
        );

        int32 target_fps = static_cast<int32>(Application::GetTargetFps());
        if (ImGui::SliderInt("Target FPS", &target_fps, 1, 2400))
        {
            Application::SetTargetFps(target_fps);
        }
    }

    if (ImGui::CollapsingHeader("Input Status", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (InputSubsystem* input = Engine::Get().GetSubsystem<InputSubsystem>())
        {
            if (ImGui::BeginTable("InputLayout", 2, ImGuiTableFlags_Resizable))
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                // --- Mouse Section ---
                if (ImGui::TreeNodeEx("Mouse", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    SE_SCOPE_DEFER{ ImGui::TreePop(); };

                    const Vector2f mouse_pos = input->GetLocalMousePosition();
                    const Vector2f wheel_delta = input->GetMouseWheel();
                    ImGui::Text("Position: (%.2f, %.2f)", mouse_pos.x, mouse_pos.y);
                    ImGui::Text("Wheel: (%.2f, %.2f)", wheel_delta.x, wheel_delta.y);

                    ImGui::SeparatorText("Delta Visualizer");
                    const ImVec2 canvas_p = ImGui::GetCursorScreenPos();
                    constexpr ImVec2 canvas_sz = ImVec2(120, 120);
                    const float center_x = canvas_p.x + (canvas_sz.x * 0.5f);
                    const float center_y = canvas_p.y + (canvas_sz.y * 0.5f);

                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    draw_list->AddRectFilled(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), IM_COL32(30, 30, 30, 255));
                    draw_list->AddRect(canvas_p, ImVec2(canvas_p.x + canvas_sz.x, canvas_p.y + canvas_sz.y), IM_COL32(100, 100, 100, 255));

                    // Grid
                    draw_list->AddLine(ImVec2(center_x, canvas_p.y), ImVec2(center_x, canvas_p.y + canvas_sz.y), IM_COL32(60, 60, 60, 255));
                    draw_list->AddLine(ImVec2(canvas_p.x, center_y), ImVec2(canvas_p.x + canvas_sz.x, center_y), IM_COL32(60, 60, 60, 255));

                    // Delta Point
                    constexpr float scale = 2.0f;
                    const Vector2f mouse_delta = input->GetMouseDelta();
                    draw_list->AddCircleFilled(ImVec2(center_x + (mouse_delta.x * scale), center_y + (mouse_delta.y * scale)), 3.0f, IM_COL32(255, 255, 0, 255));
                    draw_list->AddLine(ImVec2(center_x, center_y), ImVec2(center_x + (mouse_delta.x * scale), center_y + (mouse_delta.y * scale)), IM_COL32(255, 255, 0, 150));

                    ImGui::Dummy(canvas_sz);
                    ImGui::Text("Delta: (%.2f, %.2f)", mouse_delta.x, mouse_delta.y);

                    ImGui::SeparatorText("Buttons");
                    auto DrawMouseButton = [&](const char* label, EMouseButton btn)
                    {
                        const bool is_down = input->IsMouseButtonDown(btn);
                        if (is_down)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
                        }
                        ImGui::Button(label, ImVec2(40, 40));
                        if (is_down)
                        {
                            ImGui::PopStyleColor();
                        }
                    };

                    DrawMouseButton("L", EMouseButton::Left); ImGui::SameLine();
                    DrawMouseButton("M", EMouseButton::Middle); ImGui::SameLine();
                    DrawMouseButton("R", EMouseButton::Right);
                    DrawMouseButton("X1", EMouseButton::X1); ImGui::SameLine();
                    DrawMouseButton("X2", EMouseButton::X2);

                    ImGui::Separator();

                    bool visible = input->IsCursorVisible();
                    if (ImGui::Checkbox("Cursor Visible", &visible))
                    {
                        input->SetCursorVisible(visible);
                    }

                    bool relative = input->IsRelativeMouseMode();
                    if (ImGui::Checkbox("Relative Mode", &relative))
                    {
                        input->SetRelativeMouseMode(relative);
                    }
                }

                ImGui::TableSetColumnIndex(1);

                // --- Keyboard Section ---
                if (ImGui::TreeNodeEx("Keyboard", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    SE_SCOPE_DEFER{ ImGui::TreePop(); };

                    ImGui::Text("Active Keys:");
                    ImGui::BeginChild("KeyLog", ImVec2(0, 250), true);
                    for (uint16 i = 0; i < static_cast<uint16>(EKeyCode::Max); ++i)
                    {
                        const EKeyCode key = static_cast<EKeyCode>(i);
                        if (input->IsKeyDown(key))
                        {
                            const char* key_name = SDL_GetScancodeName(static_cast<SDL_Scancode>(i));
                            if (key_name && key_name[0] != '\0')
                            {
                                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), " [ %s ] ", key_name);
                            }
                        }
                    }
                    ImGui::EndChild();
                }

                ImGui::EndTable();
            }
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "InputSubsystem not found!");
        }
    }

    if (ImGui::CollapsingHeader("Memory Stats", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Total CPU Allocated: %s", FormatBytes(MemoryStats::GetTotalCpuAllocated()).CStr());
        ImGui::Text("Total GPU Allocated: %s", FormatBytes(MemoryStats::GetTotalGpuAllocated()).CStr());

        if (ImGui::BeginTable("MemoryTags", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            SE_SCOPE_DEFER{ ImGui::EndTable(); };

            ImGui::TableSetupColumn("Tag Name");
            ImGui::TableSetupColumn("CPU Allocated");
            ImGui::TableSetupColumn("GPU Allocated");
            ImGui::TableHeadersRow();

            for (const auto& [name, cpu_allocated, gpu_allocated] : MemoryStats::GetTags())
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(name.CStr());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(FormatBytes(cpu_allocated.load(std::memory_order_acquire)).CStr());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(FormatBytes(gpu_allocated.load(std::memory_order_acquire)).CStr());
            }
        }
    }
}
} // namespace se::editor
