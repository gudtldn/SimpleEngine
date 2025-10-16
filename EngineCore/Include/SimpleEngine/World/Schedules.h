#pragma once
#include <concepts>


namespace se::world::schedules
{
struct Schedule{};

struct PreUpdate : Schedule{};
struct Update : Schedule{};
struct PostUpdate : Schedule{};

template <typename T>
concept ScheduleType = std::derived_from<T, Schedule>;
}
