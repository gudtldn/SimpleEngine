#include "SimpleEngine/Core/Time/Time.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Transient.h"
#include "SimpleEngine/ECS/ECSAnnotations.h"


SE_REFLECT_BEGIN(se::detail::TimeState)
    SE_FIELD(delta)
    SE_FIELD(elapsed)
    SE_FIELD(frame_count)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::RealTime, se::serde::Transient, se::ecs::Resource)
    SE_BASE(se::detail::TimeState)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::GameTime, se::serde::Transient, se::ecs::Resource)
    SE_BASE(se::detail::TimeState)
    SE_FIELD(time_scale)
    SE_FIELD(paused)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::FixedTime, se::serde::Transient, se::ecs::Resource)
    SE_BASE(se::detail::TimeState)
    SE_FIELD(fixed_step)
    SE_FIELD(accumulator)
SE_REFLECT_END()
