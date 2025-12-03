#pragma once
#include <concepts>


namespace se::ecs::schedule
{
// TODO: 추후 C++26때 Custom Annotation으로 대체
struct Schedule{};

struct PreUpdate : Schedule{};
struct Update : Schedule{};
struct PostUpdate : Schedule{};

// TODO: 추후 C++26때 Custom Annotation으로 대체
template <typename T>
concept ScheduleType = std::derived_from<T, Schedule>;
}
