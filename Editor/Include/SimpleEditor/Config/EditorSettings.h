#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"


namespace se::editor
{
/**
 * [window] 섹션 - 에디터 윈도우 설정
 */
struct SE_EDITOR_API SE_ANNOTATION(=meta::SerializeOnly) WindowSettings
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
struct SE_EDITOR_API SE_ANNOTATION(=meta::SerializeOnly) EditorUISettings
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
struct SE_EDITOR_API SE_ANNOTATION(=meta::SerializeOnly) ConsoleSettings
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
 * [performance] 섹션 - 성능 설정
 */
struct SE_EDITOR_API SE_ANNOTATION(=meta::SerializeOnly) PerformanceSettings
{
    SE_ANNOTATION(=meta::Property)
    uint32 target_fps = 240;

    /** 프레임 대기 시 Busy-wait 비율 (0.0 ~ 1.0) */
    SE_ANNOTATION(=meta::Property, =meta::Range(0.0f, 1.0f))
    float busy_wait_ratio = 0.1f;

    bool operator==(const PerformanceSettings&) const = default;
};

/**
 * 프레젠트 모드 열거형
 */
enum class EPresentMode : uint8
{
    Mailbox,
    VSync,
    Immediate,
};

/**
 * [graphics] 섹션 - 렌더링 설정
 */
struct SE_EDITOR_API SE_ANNOTATION(=meta::SerializeOnly) GraphicsSettings
{
    /** 프레젠트 모드 */
    SE_ANNOTATION(=meta::Property)
    EPresentMode present_mode = EPresentMode::Mailbox;

    bool operator==(const GraphicsSettings&) const = default;
};
}  // namespace se::editor
