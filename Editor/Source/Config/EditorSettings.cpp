#include "SimpleEditor/Config/EditorSettings.h"

#include "../../../EngineCore/Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


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


// ConfigFile이 설정 파일을 읽고 쓰는 데 사용하는 등록. SettingsPanel은 위의 레거시 등록으로 그리므로 필드를 바꾸면 두 곳을 함께 고침
// 두 등록의 필드 이름과 순서는 EditorSettingsTest가 비교함
SE_REFLECT_ENUM_BEGIN(se::editor::EPresentMode)
    SE_ENUM_VALUE(Mailbox)
    SE_ENUM_VALUE(VSync)
    SE_ENUM_VALUE(Immediate)
SE_REFLECT_ENUM_END()

SE_REFLECT_BEGIN(se::editor::WindowSettings)
    SE_FIELD(title)
    SE_FIELD(width)
    SE_FIELD(height)
    SE_FIELD(fullscreen)
    SE_FIELD(borderless)
    SE_FIELD(resizable)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::EditorUISettings)
    SE_FIELD(font_path)
    SE_FIELD(font_size)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::ConsoleSettings)
    SE_FIELD(auto_scroll)
    SE_FIELD(show_timestamp)
    SE_FIELD(show_location)
    SE_FIELD(show_thread_name)
    SE_FIELD(max_log_lines)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::PerformanceSettings)
    SE_FIELD(target_fps)
    SE_FIELD(busy_wait_ratio)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::GraphicsSettings)
    SE_FIELD(present_mode)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::editor::AssetScanSettings)
    SE_FIELD(schemes)
SE_REFLECT_END()
