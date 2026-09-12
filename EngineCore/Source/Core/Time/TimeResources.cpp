#include "SimpleEngine/Core/Time/Time.h"

#include "../../../Include/SimpleEngine/Core/Reflection/Legacy/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// TODO: C++26에서 std::meta::access_context::unchecked()로 접근하면 friend가 필요 없어짐
struct [[maybe_unused]] TimeResources_Registrar
{
    SE_BEGIN_REFLECT_V1(RealTime, meta::Reflect, meta::Transient, meta::Resource)
        SE_REFLECT_PROPERTY_V1(delta, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(elapsed, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(frame_count, meta::Reflect, meta::ReadOnly)
    SE_END_REFLECT_V1(RealTime)

    SE_BEGIN_REFLECT_V1(GameTime, meta::Reflect, meta::Transient, meta::Resource)
        SE_REFLECT_PROPERTY_V1(delta, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(elapsed, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(frame_count, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(time_scale, meta::Reflect, meta::Range(0.1f, 10.0f))
        SE_REFLECT_PROPERTY_V1(paused, meta::Reflect)
    SE_END_REFLECT_V1(GameTime)

    SE_BEGIN_REFLECT_V1(FixedTime, meta::Reflect, meta::Transient, meta::Resource)
        SE_REFLECT_PROPERTY_V1(delta, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(elapsed, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(frame_count, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY_V1(fixed_step, meta::Reflect)
    SE_END_REFLECT_V1(FixedTime)
};
} // namespace se
