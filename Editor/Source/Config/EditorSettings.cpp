#include "SimpleEditor/Config/EditorSettings.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_REFLECT_ENUM(EPresentMode)

SE_BEGIN_REFLECT(WindowSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(title, meta::Reflect)
    SE_REFLECT_PROPERTY(width, meta::Reflect)
    SE_REFLECT_PROPERTY(height, meta::Reflect)
    SE_REFLECT_PROPERTY(fullscreen, meta::Reflect)
    SE_REFLECT_PROPERTY(borderless, meta::Reflect)
    SE_REFLECT_PROPERTY(resizable, meta::Reflect)
SE_END_REFLECT(WindowSettings)

SE_BEGIN_REFLECT(EditorUISettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(font_path, meta::Reflect)
    SE_REFLECT_PROPERTY(font_size, meta::Reflect)
SE_END_REFLECT(EditorUISettings)

SE_BEGIN_REFLECT(ConsoleSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(auto_scroll, meta::Reflect)
    SE_REFLECT_PROPERTY(show_timestamp, meta::Reflect)
    SE_REFLECT_PROPERTY(show_location, meta::Reflect)
    SE_REFLECT_PROPERTY(show_thread_name, meta::Reflect)
    SE_REFLECT_PROPERTY(max_log_lines, meta::Reflect)
SE_END_REFLECT(ConsoleSettings)

SE_BEGIN_REFLECT(PerformanceSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(target_fps, meta::Reflect)
    SE_REFLECT_PROPERTY(busy_wait_ratio, meta::Reflect, meta::Range(0.0f, 1.0f))
SE_END_REFLECT(PerformanceSettings)

SE_BEGIN_REFLECT(GraphicsSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(present_mode, meta::Reflect)
SE_END_REFLECT(GraphicsSettings)

SE_BEGIN_REFLECT(AssetScanSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY(schemes, meta::Reflect)
SE_END_REFLECT(AssetScanSettings)
} // namespace se::editor
