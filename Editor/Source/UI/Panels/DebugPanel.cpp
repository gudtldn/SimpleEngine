#include "UI/Panels/DebugPanel.h"

#include "imgui.h"
#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Memory/MemoryStats.h"

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

void DebugPanel::Draw()
{
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
    ImGui::Begin(GetName(), &is_visible, flags);
    SE_SCOPE_DEFER{ ImGui::End(); };

    ImGui::BeginMenuBar();
    if (ImGui::BeginMenu("Views"))
    {
        SE_SCOPE_DEFER{ ImGui::EndMenu(); };
    }
    ImGui::EndMenuBar();

    {
        static constexpr int32 max_samples = 200;
        static Array fps_history(max_samples, 0.0f);
        static int32 offset = 0;

        const float dt = static_cast<float>(Application::GetDeltaTime());
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
        if (ImGui::SliderInt("Target FPS", &target_fps, 1, 1000))
        {
            Application::SetTargetFps(target_fps);
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
}  // namespace se::editor
