#include "Config/EditorSettings.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
SE_BEGIN_REFLECT(WindowSettings, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(title, meta::Property)
    SE_REFLECT_PROPERTY(width, meta::Property)
    SE_REFLECT_PROPERTY(height, meta::Property)
    SE_REFLECT_PROPERTY(fullscreen, meta::Property)
    SE_REFLECT_PROPERTY(borderless, meta::Property)
    SE_REFLECT_PROPERTY(resizable, meta::Property)
SE_END_REFLECT(WindowSettings)

SE_BEGIN_REFLECT(EditorUISettings, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(font_path, meta::Property)
    SE_REFLECT_PROPERTY(font_size, meta::Property)
SE_END_REFLECT(EditorUISettings)

SE_BEGIN_REFLECT(ConsoleSettings, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(auto_scroll, meta::Property)
    SE_REFLECT_PROPERTY(show_timestamp, meta::Property)
    SE_REFLECT_PROPERTY(show_location, meta::Property)
    SE_REFLECT_PROPERTY(show_thread_name, meta::Property)
    SE_REFLECT_PROPERTY(max_log_lines, meta::Property)
SE_END_REFLECT(ConsoleSettings)

SE_BEGIN_REFLECT(AssetBrowserSettings, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(grid_padding, meta::Property)
    SE_REFLECT_PROPERTY(thumbnail_size, meta::Property)
    SE_REFLECT_PROPERTY(tree_column_width, meta::Property)
SE_END_REFLECT(AssetBrowserSettings)

SE_BEGIN_REFLECT(PerformanceSettings, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(target_fps, meta::Property)
    SE_REFLECT_PROPERTY(busy_wait_ratio, meta::Property, meta::Range(0.0f, 1.0f))
SE_END_REFLECT(PerformanceSettings)

SE_BEGIN_REFLECT(GraphicsSettings, meta::SerializeOnly)
    SE_REFLECT_PROPERTY(present_mode, meta::Property)
SE_END_REFLECT(GraphicsSettings)
} // namespace se::editor
