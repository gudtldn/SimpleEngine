#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


namespace se::editor
{
/**
 * [window] 섹션 - 에디터 윈도우 설정
 */
struct SE_EDITOR_API WindowSettings
{
    String title = "SimpleEngine Editor";
    u32 width = 1280;
    u32 height = 720;
    bool fullscreen = false;
    bool borderless = false;
    bool resizable = true;

    bool operator==(const WindowSettings&) const = default;
};

/**
 * [editor.ui] 섹션 - 에디터 UI 설정
 */
struct SE_EDITOR_API EditorUISettings
{
    String font_path = "CoreAssets://Font/malgun.ttf";
    f32 font_size = 17.0f;

    bool operator==(const EditorUISettings&) const = default;
};

/**
 * [editor.console] 섹션 - 콘솔 패널 설정
 */
struct SE_EDITOR_API ConsoleSettings
{
    bool auto_scroll = true;
    bool show_timestamp = false;
    bool show_thread_name = false;
    bool show_location = true;
    u32 max_log_lines = 2000;

    bool operator==(const ConsoleSettings&) const = default;
};

/**
 * [performance] 섹션 - 성능 설정
 */
struct SE_EDITOR_API PerformanceSettings
{
    u32 target_fps = 240;

    /** 프레임 대기 시 Busy-wait 비율 (0.0 ~ 1.0) */
    SE_ANNOTATE(busy_wait_ratio, display::Range(0.0f, 1.0f))
    f32 busy_wait_ratio = 0.1f;

    bool operator==(const PerformanceSettings&) const = default;
};

/**
 * 프레젠트 모드 열거형
 */
enum class EPresentMode : u8
{
    Mailbox,
    VSync,
    Immediate,
};

/**
 * [graphics] 섹션 - 렌더링 설정
 */
struct SE_EDITOR_API GraphicsSettings
{
    /** 프레젠트 모드 */
    EPresentMode present_mode = EPresentMode::Mailbox;

    bool operator==(const GraphicsSettings&) const = default;
};
/**
 * [asset_scan] 섹션 - Import 스캔 대상 VFS 스킴 목록
 */
struct SE_EDITOR_API AssetScanSettings
{
    /** 스캔할 VFS 스킴 이름 목록 */
    Array<String> schemes = { "CoreAssets", "EditorAssets" };

    bool operator==(const AssetScanSettings&) const = default;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::WindowSettings, SE_EDITOR_API)
SE_DECLARE_REFLECTION(se::editor::EditorUISettings, SE_EDITOR_API)
SE_DECLARE_REFLECTION(se::editor::ConsoleSettings, SE_EDITOR_API)
SE_DECLARE_REFLECTION(se::editor::PerformanceSettings, SE_EDITOR_API)
SE_DECLARE_REFLECTION(se::editor::EPresentMode, SE_EDITOR_API)
SE_DECLARE_REFLECTION(se::editor::GraphicsSettings, SE_EDITOR_API)
SE_DECLARE_REFLECTION(se::editor::AssetScanSettings, SE_EDITOR_API)
