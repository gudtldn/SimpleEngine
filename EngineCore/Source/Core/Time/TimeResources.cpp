#include "SimpleEngine/Core/Time/Time.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se
{
// TODO: C++26에서 std::meta::access_context::unchecked()로 접근하면 friend가 필요 없어짐
struct [[maybe_unused]] TimeResources_Registrar
{
    SE_BEGIN_REFLECT(RealTime, meta::EditorOnly, meta::Resource)
        SE_REFLECT_PROPERTY(delta, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(elapsed, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(frame_count, meta::Property, meta::ReadOnly)
    SE_END_REFLECT(RealTime)

    SE_BEGIN_REFLECT(GameTime, meta::EditorOnly, meta::Resource)
        SE_REFLECT_PROPERTY(delta, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(elapsed, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(frame_count, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(time_scale, meta::Property)
        SE_REFLECT_PROPERTY(paused, meta::Property)
    SE_END_REFLECT(GameTime)

    SE_BEGIN_REFLECT(FixedTime, meta::EditorOnly, meta::Resource)
        SE_REFLECT_PROPERTY(delta, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(elapsed, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(frame_count, meta::Property, meta::ReadOnly)
        SE_REFLECT_PROPERTY(fixed_step, meta::Property)
    SE_END_REFLECT(FixedTime)
};
} // namespace se
