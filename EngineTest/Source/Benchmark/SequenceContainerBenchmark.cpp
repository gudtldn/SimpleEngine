#include <benchmark/benchmark.h>
#include <deque>
#include <queue>
#include <stack>
#include <list>

#include "SimpleEngine/Core/Container/Deque.h"
#include "SimpleEngine/Core/Container/Queue.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Core/Container/PriorityQueue.h"

namespace se::benchmark_test
{
// --- Deque vs std::deque ---

static void BM_SE_Deque_PushBack(benchmark::State& state)
{
    for (auto _ : state)
    {
        Deque<int> deq;
        for (int i = 0; i < state.range(0); ++i)
        {
            deq.PushBack(i);
        }
        benchmark::DoNotOptimize(deq);
    }
}

BENCHMARK(BM_SE_Deque_PushBack)->Range(8, 8 << 10);

static void BM_STL_Deque_PushBack(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::deque<int> deq;
        for (int i = 0; i < state.range(0); ++i)
        {
            deq.push_back(i);
        }
        benchmark::DoNotOptimize(deq);
    }
}

BENCHMARK(BM_STL_Deque_PushBack)->Range(8, 8 << 10);

static void BM_SE_Deque_PushFront(benchmark::State& state)
{
    for (auto _ : state)
    {
        Deque<int> deq;
        for (int i = 0; i < state.range(0); ++i)
        {
            deq.PushFront(i);
        }
        benchmark::DoNotOptimize(deq);
    }
}

BENCHMARK(BM_SE_Deque_PushFront)->Range(8, 8 << 10);

static void BM_STL_Deque_PushFront(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::deque<int> deq;
        for (int i = 0; i < state.range(0); ++i)
        {
            deq.push_front(i);
        }
        benchmark::DoNotOptimize(deq);
    }
}

BENCHMARK(BM_STL_Deque_PushFront)->Range(8, 8 << 10);

// --- Queue vs std::queue ---

static void BM_SE_Queue_Push(benchmark::State& state)
{
    for (auto _ : state)
    {
        Queue<int> q;
        for (int i = 0; i < state.range(0); ++i)
        {
            q.Push(i);
        }
        benchmark::DoNotOptimize(q);
    }
}

BENCHMARK(BM_SE_Queue_Push)->Range(8, 8 << 10);

static void BM_STL_Queue_Push(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::queue<int> q;
        for (int i = 0; i < state.range(0); ++i)
        {
            q.push(i);
        }
        benchmark::DoNotOptimize(q);
    }
}

BENCHMARK(BM_STL_Queue_Push)->Range(8, 8 << 10);

// --- Stack vs std::stack ---

static void BM_SE_Stack_Push(benchmark::State& state)
{
    for (auto _ : state)
    {
        Stack<int> s;
        for (int i = 0; i < state.range(0); ++i)
        {
            s.Push(i);
        }
        benchmark::DoNotOptimize(s);
    }
}

BENCHMARK(BM_SE_Stack_Push)->Range(8, 8 << 10);

static void BM_STL_Stack_Push(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::stack<int> s;
        for (int i = 0; i < state.range(0); ++i)
        {
            s.push(i);
        }
        benchmark::DoNotOptimize(s);
    }
}

BENCHMARK(BM_STL_Stack_Push)->Range(8, 8 << 10);

// --- PriorityQueue vs std::priority_queue ---

static void BM_SE_PriorityQueue_Push(benchmark::State& state)
{
    for (auto _ : state)
    {
        PriorityQueue<int> pq;
        for (int i = 0; i < state.range(0); ++i)
        {
            pq.Push(i);
        }
        benchmark::DoNotOptimize(pq);
    }
}

BENCHMARK(BM_SE_PriorityQueue_Push)->Range(8, 8 << 10);

static void BM_STL_PriorityQueue_Push(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::priority_queue<int> pq;
        for (int i = 0; i < state.range(0); ++i)
        {
            pq.push(i);
        }
        benchmark::DoNotOptimize(pq);
    }
}

BENCHMARK(BM_STL_PriorityQueue_Push)->Range(8, 8 << 10);
}
