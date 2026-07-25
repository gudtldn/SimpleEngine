#include "SimpleEngine/Core/Time/Time.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/ECS/ECSReflectionHook.h"


namespace se
{
// TODO: C++26에서 std::meta::access_context::unchecked()로 접근하면 friend가 필요 없어짐
struct [[maybe_unused]] TimeResources_Registrar
{
    SE_BEGIN_REFLECT(RealTime, meta::Reflect, meta::Transient, meta::Resource)
        SE_REFLECT_PROPERTY(delta, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(elapsed, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(frame_count, meta::Reflect, meta::ReadOnly)
    SE_END_REFLECT(RealTime)

    SE_BEGIN_REFLECT(GameTime, meta::Reflect, meta::Transient, meta::Resource)
        SE_REFLECT_PROPERTY(delta, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(elapsed, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(frame_count, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(time_scale, meta::Reflect, meta::Range(0.1f, 10.0f))
        SE_REFLECT_PROPERTY(paused, meta::Reflect)
    SE_END_REFLECT(GameTime)

    SE_BEGIN_REFLECT(FixedTime, meta::Reflect, meta::Transient, meta::Resource)
        SE_REFLECT_PROPERTY(delta, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(elapsed, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(frame_count, meta::Reflect, meta::ReadOnly)
        SE_REFLECT_PROPERTY(fixed_step, meta::Reflect)
    SE_END_REFLECT(FixedTime)
};
} // namespace se
