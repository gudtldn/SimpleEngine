#include "UI/Panels/AssetsBrowserPanel.h"
#include "SimpleEngine/Utility/PathResolver.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "imgui.h"

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

        ImGui::Text("%s", selected_path.ValueOr("No directory selected").CStr());
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
        if (GetSelectedDirPath() == physical_path)
        {
            root_flags |= ImGuiTreeNodeFlags_Selected;
        }

        const bool root_has_sub_dirs = HasSubDirectories(physical_path);
        if (!root_has_sub_dirs)
        {
            root_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        // 트리 노드 그리기
        const bool is_node_open = ImGui::TreeNodeEx(scheme.CStr(), root_flags);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            SetSelectedDirPath(physical_path);
        }

        // 우클릿 컨텍스트 메뉴
        if (ImGui::BeginPopupContextItem())
        {
            DrawDirectoryContextMenu(physical_path);
            ImGui::EndPopup();
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
    SE_TODO("AssetsBrowserPanel::DrawAssetGrid 구현하기"); // TODO: AssetsBrowserPanel::DrawAssetGrid 구현하기
}

bool AssetsBrowserPanel::HasSubDirectories(const fs::path& path)
{
    std::error_code ec;
    return std::ranges::any_of(fs::directory_iterator(path, ec), [&ec, &path](const auto& entry)
    {
        if (ec)
        {
            ConsoleLog(ELogLevel::Warning, "Failed to read directory: {}", path.string());
            return false;
        }
        return entry.is_directory();
    });
}

void AssetsBrowserPanel::RenderDirectoryTreeRecursive(const fs::path& path)
{
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(path, ec))
    {
        if (ec)
        {
            ConsoleLog(ELogLevel::Warning, "Failed to read directory: {}", path.string());
            continue;
        }

        if (!entry.is_directory())
        {
            continue;
        }

        const fs::path& entry_path = entry.path();
        String folder_name = utility::ToString(entry_path.filename().c_str());

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

        // 트리 노드 그리기
        const bool is_node_open = ImGui::TreeNodeEx(folder_name.CStr(), flags);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            SetSelectedDirPath(entry_path);
        }

        // 우클릭 컨텍스트 메뉴
        if (ImGui::BeginPopupContextItem())
        {
            DrawDirectoryContextMenu(entry_path);
            ImGui::EndPopup();
        }

        if (is_node_open && has_subdirectories)
        {
            RenderDirectoryTreeRecursive(entry_path);
            ImGui::TreePop();
        }
    }
}

void AssetsBrowserPanel::DrawDirectoryContextMenu(const std::filesystem::path& path)
{
    // 메뉴 타이틀
    ImGui::TextDisabled("%s", utility::ToString(path.filename().c_str()).CStr());
    ImGui::Separator();

    if (ImGui::MenuItem("Import Asset Here..."))
    {
        // TODO: 파일 다이얼로그 열기 및 Import 로직 연결
        // FileDialog::OpenFile(..., path.string().c_str());
        ConsoleLog(ELogLevel::Info, "Import request at: {}", path.string());
    }

    if (ImGui::MenuItem("Create New Folder"))
    {
        // TODO: 폴더 생성 로직
        ConsoleLog(ELogLevel::Info, "Create new folder at: {}", path.string());
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer"))
    {
        // 플랫폼 별 탐색기 열기 (ShellExecute 등)
        // se::core::Platform::RevealInExplorer(path); 구현 필요
        ConsoleLog(ELogLevel::Info, "Show in Explorer: {}", path.string());
    }
}
}
