#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se::editor
{
/**
 * [window] 섹션 - 에디터 윈도우 설정
 */
struct SE_ANNOTATION(=meta::SerializeOnly) WindowSettings
{
    SE_ANNOTATION(=meta::Property)
    String title = "SimpleEngine Editor";

    SE_ANNOTATION(=meta::Property)
    uint32 width = 1280;

    SE_ANNOTATION(=meta::Property)
    uint32 height = 720;

    SE_ANNOTATION(=meta::Property)
    bool fullscreen = false;

    SE_ANNOTATION(=meta::Property)
    bool borderless = false;

    SE_ANNOTATION(=meta::Property)
    bool resizable = true;

    bool operator==(const WindowSettings&) const = default;
};

/**
 * [editor.ui] 섹션 - 에디터 UI 설정
 */
struct SE_ANNOTATION(=meta::SerializeOnly) EditorUISettings
{
    SE_ANNOTATION(=meta::Property)
    String font_path = "CoreAssets://Font/malgun.ttf";

    SE_ANNOTATION(=meta::Property)
    float font_size = 17.0f;

    bool operator==(const EditorUISettings&) const = default;
};

/**
 * [editor.console] 섹션 - 콘솔 패널 설정
 */
struct SE_ANNOTATION(=meta::SerializeOnly) ConsoleSettings
{
    SE_ANNOTATION(=meta::Property)
    bool auto_scroll = true;

    SE_ANNOTATION(=meta::Property)
    bool show_timestamp = false;

    SE_ANNOTATION(=meta::Property)
    bool show_thread_name = false;

    SE_ANNOTATION(=meta::Property)
    bool show_location = true;

    SE_ANNOTATION(=meta::Property)
    uint32 max_log_lines = 2000;

    bool operator==(const ConsoleSettings&) const = default;
};

/**
 * [editor.asset_browser] 섹션 - 에셋 브라우저 설정
 */
struct SE_ANNOTATION(=meta::SerializeOnly) AssetBrowserSettings
{
    SE_ANNOTATION(=meta::Property)
    float grid_padding = 16.0f;

    SE_ANNOTATION(=meta::Property)
    float thumbnail_size = 64.0f;

    SE_ANNOTATION(=meta::Property)
    float tree_column_width = 150.0f;

    bool operator==(const AssetBrowserSettings&) const = default;
};

/**
 * [performance] 섹션 - 성능 설정
 */
struct SE_ANNOTATION(=meta::SerializeOnly) PerformanceSettings
{
    SE_ANNOTATION(=meta::Property)
    uint32 target_fps = 240;

    /** 프레임 대기 시 Busy-wait 비율 (0.0 ~ 1.0) */
    SE_ANNOTATION(=meta::Property, =meta::Range(0.0f, 1.0f))
    float busy_wait_ratio = 0.1f;

    bool operator==(const PerformanceSettings&) const = default;
};

/**
 * [graphics] 섹션 - 렌더링 설정
 */
struct SE_ANNOTATION(=meta::SerializeOnly) GraphicsSettings
{
    /** 프레젠트 모드: "mailbox", "vsync", "immediate" */
    SE_ANNOTATION(=meta::Property)
    String present_mode = "mailbox"; // TODO: 나중에 Enum으로 변경

    bool operator==(const GraphicsSettings&) const = default;
};
}  // namespace se::editor
