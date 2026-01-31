#include "UI/Panels/AssetsBrowserPanel.h"

#include <compare>

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Utility/FileSystem.h"
#include "SimpleEngine/Utility/VFS.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "imgui.h"

namespace
{
struct AssetItem
{
    se::Path path;
    se::String name;
    bool is_directory;

    std::strong_ordering operator<=>(const AssetItem& other) const noexcept
    {
        if (is_directory == other.is_directory)
        {
            return is_directory <=> other.is_directory;
        }
        return name <=> other.name;
    }
};
}  // namespace

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

        const Optional<String> selected_path = GetSelectedDirPath().ToVirtual()
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
    VFS::Get().VisitMounts([this](std::string_view scheme, const Path& physical_path, [[maybe_unused]] int32 priority)
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
        const bool is_node_open = ImGui::TreeNodeEx(String(scheme).CStr(), root_flags);
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
    const Path& current_path = GetSelectedDirPath();

    // 경로 유효성 검사
    if (current_path.IsEmpty() || !current_path.Exists())
    {
        return;
    }

    // 그리드 레이아웃 설정 | TODO: 나중에 따로 설정으로 빼놓기
    static float padding = 16.0f;
    static float thumbnail_size = 64.0f;
    const float cell_size = thumbnail_size + padding;

    const float panel_width = ImGui::GetContentRegionAvail().x;
    const int column_count = std::max(static_cast<int>(panel_width / cell_size), 1);

    // 파일 목록 수집
    Array<AssetItem> items;
    for (const auto& entry : FileSystem::ReadDir(current_path))
    {
        Path item_path = entry.GetPath();

        // .meta 파일 숨기기
        if (item_path.Extension().ValueOrDefault() == ".meta")
        {
            continue;
        }

        String item_name = item_path.FileName().ValueOr("(Unknown)");
        items.Push({
            .path = std::move(item_path),
            .name = std::move(item_name),
            .is_directory = entry.IsDirectory()
        });
    }

    // Item 정렬
    std::sort(items.begin(), items.end()); // NOLINT(*-use-ranges)

    // 테이블(그리드) 그리기
    if (ImGui::BeginTable("AssetGridTable", column_count))
    {
        for (const AssetItem& item : items)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(item.name.CStr());

            // 스타일링: 아이콘처럼 보이게 큰 버튼 사용
            // TODO: 나중에 실제 아이콘 텍스처(folder_icon, file_icon)로 교체 필요
            // ImGui::ImageButton(...)

            // 폴더/파일 색상 구분 (임시)
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                item.is_directory
                    ? ImVec4(0.8f, 0.7f, 0.3f, 0.5f)
                    : ImVec4(0.3f, 0.5f, 0.8f, 0.5f)
            );

            // 버튼 그리기 (썸네일)
            if (ImGui::Button(item.is_directory ? "[Dir]" : "[File]", ImVec2(thumbnail_size, thumbnail_size)))
            {
                // 싱글 클릭: 선택 처리 (필요시)
            }
            ImGui::PopStyleColor();

            // Drag & Drop Source (파일인 경우)
            if (!item.is_directory && ImGui::BeginDragDropSource())
            {
                // 텍스처 등 에셋 로딩을 위해 경로 전달
                const String item_path = item.path.ToString();
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", item_path.CStr(), item_path.ByteLen() + 1);
                ImGui::Text("%s", item.name.CStr()); // 드래그 중 힌트
                ImGui::EndDragDropSource();
            }

            // 더블 클릭 처리
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                if (item.is_directory)
                {
                    SetSelectedDirPath(item.path);
                }
                else
                {
                    // 파일 열기 (임시: 로그 출력)
                    // TODO: AssetEditor 등을 열거나 외부 프로그램 연결
                    ConsoleLog(ELogLevel::Info, "Open File: {}", item.name);
                }
            }

            // 우클릭 컨텍스트 메뉴
            if (ImGui::BeginPopupContextItem())
            {
                if (item.is_directory)
                {
                    DrawDirectoryContextMenu(item.path);
                }
                else
                {
                    DrawFileContextMenu(item.path);
                }
                ImGui::EndPopup();
            }

            // 파일 이름 텍스트 (가운데 정렬, 래핑)
            // 텍스트가 썸네일 너비를 넘어가면 줄바꿈
            ImGui::PushItemWidth(thumbnail_size);
            ImGui::TextWrapped("%s", item.name.CStr());
            ImGui::PopItemWidth();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

bool AssetsBrowserPanel::HasSubDirectories(const Path& path)
{
    for (const auto& entry : FileSystem::ReadDir(path))
    {
        if (entry.IsDirectory())
        {
            return true;
        }
    }
    return false;
}

const Path& AssetsBrowserPanel::GetSelectedDirPath() const noexcept
{
    return selected_dir_path;
}

void AssetsBrowserPanel::SetSelectedDirPath(const Path& new_path) noexcept
{
    selected_dir_path = new_path;
}

void AssetsBrowserPanel::RenderDirectoryTreeRecursive(const Path& path)
{
    for (const auto& entry : FileSystem::ReadDir(path))
    {
        if (!entry.IsDirectory())
        {
            continue;
        }

        const Path entry_path = entry.GetPath();
        const String folder_name = entry_path.FileName().ValueOr("(Unknown)");

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

void AssetsBrowserPanel::DrawDirectoryContextMenu(const Path& path)
{
    // 메뉴 타이틀
    ImGui::TextDisabled("%s", path.FileName().ValueOr("(Unknown)").CStr());
    ImGui::Separator();

    if (ImGui::MenuItem("Import Asset Here..."))
    {
        // TODO: 파일 다이얼로그 열기 및 Import 로직 연결
        // FileDialog::OpenFile(..., path.string().c_str());
        ConsoleLog(ELogLevel::Info, "Import request at: {}", path);
    }

    if (ImGui::MenuItem("Create New Folder"))
    {
        // TODO: 폴더 생성 로직
        ConsoleLog(ELogLevel::Info, "Create new folder at: {}", path);
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer"))
    {
        ConsoleLog(ELogLevel::Info, "Show in Explorer: {}", path);
        platform::RevealInExplorer(path);
    }
}

void AssetsBrowserPanel::DrawFileContextMenu(const Path& path)
{
    ImGui::TextDisabled("%s", path.FileName().ValueOr("(Unknown)").CStr());
    ImGui::Separator();

    if (ImGui::MenuItem("Open"))
    {
        // TODO: 에셋 타입에 맞는 에디터 열기
        ConsoleLog(ELogLevel::Info, "Opening asset: {}", path);
    }

    if (ImGui::MenuItem("Delete"))
    {
        // TODO: 삭제 확인 팝업 후 삭제 로직
        ConsoleLog(ELogLevel::Warning, "Delete requested: {}", path);
        // fs::remove(path); // 위험하므로 실제 구현 시 주의
    }

    if (ImGui::MenuItem("Rename"))
    {
        // TODO: 이름 변경 로직
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show in Explorer"))
    {
        ConsoleLog(ELogLevel::Info, "Show in Explorer: {}", path);
        platform::RevealInExplorer(path);
    }
}
}  // namespace se::editor::ui
