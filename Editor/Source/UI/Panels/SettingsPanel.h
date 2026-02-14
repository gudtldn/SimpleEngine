#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Config/EditorSettings.h"


namespace se::editor
{
/**
 * 에디터/프로젝트 설정을 관리하는 패널입니다.
 * EngineConfig.toml 파일을 읽고 쓰며, 리플렉션 구조체 기반으로 설정을 편집합니다.
 *
 * 좌측: 설정 카테고리 목록 (Window, UI, Console, Asset Browser, Performance, Graphics)
 * 우측: 선택된 카테고리의 설정 편집기
 */
class SettingsPanel : public IEditorPanel
{
public:
    SettingsPanel();

    [[nodiscard]] virtual const char* GetName() const override;
    virtual void Draw() override;

    enum class ECategory : int32
    {
        Window = 0,
        UI,
        Console,
        AssetBrowser,
        Performance,
        Graphics,
        COUNT,
    };

private:
    /** 설정 파일을 로드하여 각 구조체에 채웁니다. */
    void LoadSettings();

    /** 현재 구조체 값을 파일에 저장합니다. */
    void SaveSettings();

    /** 각 카테고리별 UI 그리기 */
    void DrawWindowSettings();
    void DrawUISettings();
    void DrawConsoleSettings();
    void DrawAssetBrowserSettings();
    void DrawPerformanceSettings();
    void DrawGraphicsSettings();

private:
    ECategory current_category = ECategory::Window;
    bool needs_save = false;
    bool needs_reload = true;

    // 설정 구조체 인스턴스
    WindowSettings window_settings;
    EditorUISettings ui_settings;
    ConsoleSettings console_settings;
    AssetBrowserSettings asset_browser_settings;
    PerformanceSettings performance_settings;
    GraphicsSettings graphics_settings;
};
}  // namespace se::editor
