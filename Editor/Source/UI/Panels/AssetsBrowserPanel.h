#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEditor/Asset/MetaFileContent.h"

#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
// forward declaration
class EditorSelection;

class AssetsBrowserPanel : public IEditorPanel
{
public:
    AssetsBrowserPanel();

public:
    [[nodiscard]] virtual const char* GetName() const override;

protected:
    [[nodiscard]] virtual ImGuiWindowFlags GetWindowFlags() const override;
    virtual void DrawContent() override;

protected:
    virtual void DrawAssetTree();
    virtual void DrawAssetGrid();

protected:
    [[nodiscard]] static bool HasSubDirectories(const Path& path);

private:
    void RenderDirectoryTreeRecursive(const Path& path);
    void DrawDirectoryContextMenu(const Path& path);
    void DrawFileContextMenu(const Path& path);

    /** 파일에 포함된 StaticMesh 에셋들을 World에 Spawn합니다. */
    void SpawnMeshEntitiesFromFile(const Path& file_path);

    // Import Settings 모달
    void OpenImportSettingsModal(const Path& asset_path);
    void DrawImportSettingsModal();
    bool DrawImportSettings();
    bool DrawProcessorStack();

private:
    EditorSelection& editor_selection;

    // Import Settings 모달 상태
    Path modal_asset_path;                     // Import Settings를 띄울 에셋의 경로
    Optional<MetaFileContent> modal_content;   // 에셋의 Import Settings 정보
    bool modal_dirty = false;                  // Import Settings가 수정되었는지 여부
    bool pending_open_import_settings = false; // Modal을 띄우기 위한 Flag
};
} // namespace se::editor
