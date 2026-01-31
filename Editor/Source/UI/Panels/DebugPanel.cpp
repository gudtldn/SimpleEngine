#include "UI/Panels/DebugPanel.h"

#include "imgui.h"
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

namespace se::editor::ui
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
    {
        ImGui::BeginMenuBar();
        if (ImGui::BeginMenu("Views"))
        {
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();

        if (ImGui::CollapsingHeader("Memory Stats", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Total CPU Allocated: %s", FormatBytes(MemoryStats::GetTotalCpuAllocated()).CStr());
            ImGui::Text("Total GPU Allocated: %s", FormatBytes(MemoryStats::GetTotalGpuAllocated()).CStr());

            if (ImGui::BeginTable("MemoryTags", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
            {
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
                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
}
}  // namespace se::editor::ui
