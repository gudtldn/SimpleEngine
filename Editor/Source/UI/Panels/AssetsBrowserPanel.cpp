#include "UI/Panels/AssetsBrowserPanel.h"
#include "SimpleEngine/Utility/PathResolver.h"

#include "imgui.h"
#include "SimpleEngine/Utility/StringUtils.h"

namespace fs = std::filesystem;


namespace se::editor::ui
{
const char* AssetsBrowserPanel::GetName() const
{
    return "AssetsBrowser";
}

void AssetsBrowserPanel::Draw()
{
    ImGui::Begin(GetName(), &is_visible, ImGuiWindowFlags_MenuBar);

    // TreeView | GridView를 분할
    if (ImGui::BeginTable("AssetsBrowser_PanelSplit", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        // 좌측 컬럼 너비 초기값 설정
        ImGui::TableSetupColumn("Tree", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);

        // TreeView
        ImGui::TableNextColumn();
        ImGui::BeginChild("TreeView");
        {
            DrawAssetTree();
        }
        ImGui::EndChild();

        // GridView
        ImGui::TableNextColumn();

        const Optional<String> selected_path = utility::PathResolver::Get().Unresolve(GetSelectedDirPath())
            .AndThen([](const auto& vpath) -> Optional<String>
            {
                return vpath.ToString();
            });

        ImGui::Text("%s", selected_path.ValueOr("").CStr());
        ImGui::Separator();

        ImGui::BeginChild("GridView");
        {
            DrawAssetGrid();
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    ImGui::End();
}

void AssetsBrowserPanel::DrawAssetTree()
{
    utility::PathResolver::Get().VisitMountPoints([this](const StringName& scheme, const fs::path& physical_path, [[maybe_unused]] int32 priority)
    {
        ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

        const bool root_has_sub_dirs = HasSubDirectories(physical_path);
        if (!root_has_sub_dirs)
        {
            root_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        const bool is_node_open = ImGui::TreeNodeEx(scheme.CStr(), root_flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SetSelectedDirPath(physical_path);
        }

        if (is_node_open && root_has_sub_dirs)
        {
            RenderDirectoryTreeRecursive(physical_path);
            ImGui::TreePop();
        }
    });
}

void AssetsBrowserPanel::DrawAssetGrid()
{
}

bool AssetsBrowserPanel::HasSubDirectories(const fs::path& path)
{
    std::error_code ec;
    return std::ranges::any_of(fs::directory_iterator(path, ec), [](const auto& entry)
    {
        return entry.is_directory();
    });
}

void AssetsBrowserPanel::RenderDirectoryTreeRecursive(const fs::path& path)
{
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(path, ec))
    {
        if (!entry.is_directory())
        {
            continue;
        }

        const fs::path& entry_path = entry.path();
        String folder_name = utility::string::ToString(entry_path.filename().c_str());

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (GetSelectedDirPath() == entry_path)
        {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool has_subdirectories = HasSubDirectories(entry_path);
        if (!has_subdirectories)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        const bool is_node_open = ImGui::TreeNodeEx(folder_name.CStr(), flags);
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
        {
            SetSelectedDirPath(entry_path);
        }

        if (is_node_open && has_subdirectories)
        {
            RenderDirectoryTreeRecursive(entry_path);
            ImGui::TreePop();
        }
    }
}
}
