#pragma once
#include <concepts>


namespace se::ecs
{
// TODO: 추후 C++26때 Custom Annotation으로 대체
struct Phase{};

struct PreUpdatePhase : Phase{};
struct UpdatePhase : Phase{};
struct PostUpdatePhase : Phase{};

// TODO: 추후 C++26때 Custom Annotation으로 대체
template <typename T>
concept PhaseType = std::derived_from<T, Phase>;
}  // namespace se::ecs
