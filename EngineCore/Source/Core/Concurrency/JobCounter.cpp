// ReSharper disable CppDFAMemoryLeak
#include "SimpleEngine/Core/Concurrency/JobHandle.h"

#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Debug.h"

#include <thread>


namespace se
{
JobCounter::JobCounter(usize in_initial_count)
    : count(in_initial_count)
{
    if (in_initial_count == 0)
    {
        // 완료 상태로 설정
        waiters.store(CompletedSentinel(), std::memory_order_relaxed);
    }
}

JobCounter::~JobCounter()
{
    // 아직 남아있는 Waiter가 있으면 해제 (정상적인 상황에서는 발생하지 않아야 함)
    WaiterNode* list = waiters.load(std::memory_order_acquire);
    if (list && list != CompletedSentinel())
    {
        ConsoleLog(ELogLevel::Warning, "JobCounter destroyed with active waiters");

        while (list)
        {
            WaiterNode* next = list->next;
            delete list;
            list = next;
        }
    }
}

void JobCounter::Decrement()
{
    const usize prev = count.fetch_sub(1, std::memory_order_acq_rel);
    SE_ASSERT(prev > 0, "Decrement called on already-zero counter");

    if (prev == 1)
    {
        // Waiter 리스트를 COMPLETED로 교체하고 모든 Waiter에 대해 Notify 호출
        WaiterNode* list = waiters.exchange(CompletedSentinel(), std::memory_order_acq_rel);
        NotifyWaiters(list);
    }
}

bool JobCounter::IsComplete() const
{
    return count.load(std::memory_order_acquire) == 0;
}

usize JobCounter::GetCount() const
{
    return count.load(std::memory_order_acquire);
}

Optional<std::coroutine_handle<>> JobCounter::AddWaiter(std::coroutine_handle<> in_handle)
{
    WaiterNode* node = new WaiterNode();
    node->coroutine = in_handle;

    WaiterNode* old_head = waiters.load(std::memory_order_acquire);
    do
    {
        // 이미 완료된 상태인지?
        if (old_head == CompletedSentinel())
        {
            delete node;
            return in_handle;
        }
        node->next = old_head;
    }
    while (!waiters.compare_exchange_weak(
        old_head, node,
        std::memory_order_release, std::memory_order_acquire
    ));

    return NullOpt;
}

Optional<UniqueFunction<void()>> JobCounter::AddWaiter(UniqueFunction<void()>&& in_callback)
{
    WaiterNode* node = new WaiterNode();
    node->callback = std::move(in_callback);

    WaiterNode* old_head = waiters.load(std::memory_order_acquire);
    do
    {
        // 이미 완료된 상태인지?
        if (old_head == CompletedSentinel())
        {
            in_callback = std::move(node->callback);
            delete node;

            return in_callback;
        }
        node->next = old_head;
    }
    while (!waiters.compare_exchange_weak(
        old_head, node,
        std::memory_order_release, std::memory_order_acquire
    ));

    return NullOpt;
}

void* JobCounter::WaiterNode::operator new(usize size)
{
    return JobAllocator::Allocate(size);
}

void JobCounter::WaiterNode::operator delete(void* ptr)
{
    JobAllocator::Free(ptr);
}

JobCounter::WaiterNode* JobCounter::CompletedSentinel()
{
    // 유효한 힙/스택 주소가 될 수 없는 값. 비교 전용으로 역참조시 UB
    return reinterpret_cast<WaiterNode*>(static_cast<usize>(1));
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void JobCounter::NotifyWaiters(WaiterNode* in_list)
{
    while (in_list && in_list != CompletedSentinel())
    {
        WaiterNode* next = in_list->next;

        if (in_list->coroutine)
        {
            in_list->coroutine.resume();
        }
        else if (in_list->callback)
        {
            in_list->callback();
        }

        delete in_list;
        in_list = next;
    }
}

void JobHandle::Wait() const
{
    if (!counter)
    {
        return;
    }

    // JobSystem이 초기화되지 않은 환경에서는 단순 Spin-Yield로 대기
    const bool has_job_system = JobSystem::IsInitialized();

    while (!counter->IsComplete())
    {
        if (has_job_system && JobSystem::Get().TryExecuteOne())
        {
            continue;
        }
        std::this_thread::yield();
    }
}
} // namespace se
