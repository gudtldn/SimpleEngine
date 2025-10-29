#pragma once
#include <queue>
#include <stack>

#include "SimpleEngine/Core/Container/Deque.h"


// pmr container wrapper
namespace se
{
template <typename T>
using queue [[deprecated]] = std::queue<T, std::deque<T>>;

template <typename T>
using stack [[deprecated]] = std::stack<T, std::deque<T>>;
}

