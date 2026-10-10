#include "SimpleEditor/Config/EditorSettings.h"

#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


SE_REFLECT_ENUM_BEGIN(se::editor::EPresentMode)
    SE_ENUM_VALUE(Mailbox)
    SE_ENUM_VALUE(VSync)
    SE_ENUM_VALUE(Immediate)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se::editor::WindowSettings, se::display::Hidden)
    SE_FIELD(title)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(fullscreen)
    SE_FIELD(borderless)
    SE_FIELD(resizable)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::EditorUISettings, se::display::Hidden)
    SE_FIELD(font_path)
    SE_FIELD(font_size)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::ConsoleSettings, se::display::Hidden)
    SE_FIELD(auto_scroll)
    SE_FIELD(show_timestamp)
    SE_FIELD(show_location)
    SE_FIELD(show_thread_name)
    SE_FIELD(max_log_lines)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::PerformanceSettings, se::display::Hidden)
    SE_FIELD(target_fps)
    SE_FIELD(busy_wait_ratio)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::GraphicsSettings, se::display::Hidden)
    SE_FIELD(present_mode)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::AssetScanSettings, se::display::Hidden)
    SE_FIELD(schemes)
SE_REFLECT_END()
