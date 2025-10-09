export module SE.Core:ECS.Schedules;

import std;


export namespace se::core::ecs::schedules
{
struct Schedule {};

struct PreUpdate : Schedule {};
struct Update : Schedule {};
struct PostUpdate : Schedule {};

template <typename T>
concept ScheduleType = std::derived_from<T, Schedule>;
}
