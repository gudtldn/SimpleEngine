#pragma once

#include "SimpleEngine/Core/Types/Path.h"
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
    [[nodiscard]] static bool HasSubDirectories(const Path& path);

    [[nodiscard]] const Path& GetSelectedDirPath() const noexcept;
    void SetSelectedDirPath(const Path& new_path) noexcept;

private:
    void RenderDirectoryTreeRecursive(const Path& path);
    void DrawDirectoryContextMenu(const Path& path);

    void DrawFileContextMenu(const Path& path);

private:
    Path selected_dir_path;
};
}  // namespace se::editor::ui
