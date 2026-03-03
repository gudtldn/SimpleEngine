#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEditor/Asset/MetaFileContent.h"

#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
class AssetsBrowserPanel : public IEditorPanel
{
public:
    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

protected:
    virtual void DrawAssetTree();
    virtual void DrawAssetGrid();

protected:
    [[nodiscard]] static bool HasSubDirectories(const Path& path);

    [[nodiscard]] const Path& GetSelectedDirPath() const noexcept;
    void SetSelectedDirPath(const Path& new_path) noexcept;

private:
    void RenderDirectoryTreeRecursive(const Path& path);
    void DrawDirectoryContextMenu(const Path& path);
    void DrawFileContextMenu(const Path& path);

    // Import Settings 모달
    void OpenImportSettingsModal(const Path& asset_path);
    void DrawImportSettingsModal();
    bool DrawImportSettings();
    bool DrawProcessorStack();

private:
    // 선택된 Directory 경로
    Path selected_dir_path;

    // Import Settings 모달 상태
    Path modal_asset_path;                     // Import Settings를 띄울 에셋의 경로
    Optional<MetaFileContent> modal_content;   // 에셋의 Import Settings 정보
    bool modal_dirty = false;                  // Import Settings가 수정되었는지 여부
    bool pending_open_import_settings = false; // Modal을 띄우기 위한 Flag
};
}  // namespace se::editor
