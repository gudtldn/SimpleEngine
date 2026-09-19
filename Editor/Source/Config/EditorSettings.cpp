#include "SimpleEditor/Config/EditorSettings.h"

#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"


namespace se::editor
{
SE_REFLECT_ENUM_V1(EPresentMode)

SE_BEGIN_REFLECT_V1(WindowSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(title, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(width, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(height, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(fullscreen, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(borderless, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(resizable, meta::Reflect)
SE_END_REFLECT_V1(WindowSettings)

SE_BEGIN_REFLECT_V1(EditorUISettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(font_path, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(font_size, meta::Reflect)
SE_END_REFLECT_V1(EditorUISettings)

SE_BEGIN_REFLECT_V1(ConsoleSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(auto_scroll, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(show_timestamp, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(show_location, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(show_thread_name, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(max_log_lines, meta::Reflect)
SE_END_REFLECT_V1(ConsoleSettings)

SE_BEGIN_REFLECT_V1(PerformanceSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(target_fps, meta::Reflect)
    SE_REFLECT_PROPERTY_V1(busy_wait_ratio, meta::Reflect, meta::Range(0.0f, 1.0f))
SE_END_REFLECT_V1(PerformanceSettings)

SE_BEGIN_REFLECT_V1(GraphicsSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(present_mode, meta::Reflect)
SE_END_REFLECT_V1(GraphicsSettings)

SE_BEGIN_REFLECT_V1(AssetScanSettings, meta::Reflect, meta::Hidden)
    SE_REFLECT_PROPERTY_V1(schemes, meta::Reflect)
SE_END_REFLECT_V1(AssetScanSettings)
} // namespace se::editor
