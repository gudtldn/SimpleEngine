#pragma once
#include <filesystem>

#include "UI/Panels/IEditorPanel.h"


namespace se::editor::ui
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
    [[nodiscard]] static bool HasSubDirectories(const std::filesystem::path& path);

    [[nodiscard]] const std::filesystem::path& GetSelectedDirPath() const noexcept;
    void SetSelectedDirPath(const std::filesystem::path& new_path) noexcept;

private:
    void RenderDirectoryTreeRecursive(const std::filesystem::path& path);
    void DrawDirectoryContextMenu(const std::filesystem::path& path);

    void DrawFileContextMenu(const std::filesystem::path& path);

private:
    std::filesystem::path selected_dir_path;
};
}
