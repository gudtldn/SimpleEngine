#pragma once

#include "SimpleEditor/UI/IEditorPanel.h"
#include "SimpleEditor/Config/EditorSettings.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se::editor
{
/**
 * 에디터/프로젝트 설정을 관리하는 패널입니다.
 * EngineConfig.toml 파일을 읽고 쓰며, 리플렉션 구조체 기반으로 설정을 편집합니다.
 *
 * 좌측: 설정 카테고리 목록
 * 우측: 선택된 카테고리의 설정 편집기
 */
class SettingsPanel : public IEditorPanel
{
    enum class ECategory : uint8
    {
        Window,
        UI,
        Console,
        Performance,
        Graphics,
    };

public:
    SettingsPanel();

public:
    [[nodiscard]] virtual const char* GetName() const override;

protected:
    [[nodiscard]] virtual ImGuiWindowFlags GetWindowFlags() const override;
    virtual void DrawContent() override;

private:
    /** 설정 파일을 로드하여 각 구조체에 채웁니다. */
    void LoadSettings();

    /** 현재 구조체 값을 파일에 저장합니다. */
    void SaveSettings();

    /** Category UI 그리기 */
    [[nodiscard]] static bool DrawSettings(const char* label, const TypeId& type_id, void* settings_ptr);

private:
    ECategory current_category = ECategory::Window;
    bool needs_save = false;
    bool needs_reload = true;

    // 설정 구조체 인스턴스
    WindowSettings window_settings;
    EditorUISettings ui_settings;
    ConsoleSettings console_settings;
    PerformanceSettings performance_settings;
    GraphicsSettings graphics_settings;
};
} // namespace se::editor
