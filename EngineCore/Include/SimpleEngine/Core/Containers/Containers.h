#pragma once
#include <queue>
#include <stack>

#include "SimpleEngine/Core/Container/Deque.h"


// pmr container wrapper
namespace se
{
template <typename T>
using queue [[deprecated]] = std::queue<T, Deque<T>>;

template <typename T>
using stack [[deprecated]] = std::stack<T, Deque<T>>;
}

