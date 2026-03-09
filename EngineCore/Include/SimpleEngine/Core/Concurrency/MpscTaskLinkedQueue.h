#pragma once

#include "SimpleEngine/Core/Concurrency/Common.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include <atomic>


namespace se
{
/**
 * Lock-Free MPSC (Multi-Producer, Single-Consumer) Queue
 *
 * 여러 생산자(Producer)가 동시에 Push()하고, 단일 소비자(Consumer)가 Drain()으로 일괄 소비하는 구조에 최적화되어 있습니다.
 *
 * Dmitry Vyukov's non-intrusive lock free unbound MPSC queue
 * https://web.archive.org/web/20210124182030/http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
 *
 * @note 특정 스레드(메인 스레드, 렌더 스레드 등)에 작업을 위임할 때 주로 사용됩니다.
 */
class SE_CORE_API MpscTaskLinkedQueue
{
public:
    MpscTaskLinkedQueue();
    ~MpscTaskLinkedQueue();

    // 복사 & 이동 금지
    MpscTaskLinkedQueue(const MpscTaskLinkedQueue&) = delete;
    MpscTaskLinkedQueue& operator=(const MpscTaskLinkedQueue&) = delete;
    MpscTaskLinkedQueue(MpscTaskLinkedQueue&&) = delete;
    MpscTaskLinkedQueue& operator=(MpscTaskLinkedQueue&&) = delete;

public:
    /**
     * 작업을 큐에 삽입합니다. (모든 스레드에서 호출 가능)
     * @param in_work 실행할 함수 객체
     */
    void Push(UniqueFunction<void()>&& in_work);

    /**
     * 현재 큐에 쌓인 모든 작업을 순차적으로 실행하고 큐를 비웁니다.
     * @return 실행된 작업의 수
     * @warning 반드시 Queue를 소유한 스레드(Owner Thread)에서만 호출해야 합니다.
     */
    uint32 Drain();

    /**
     * 큐가 비어있는지 대략적으로 확인합니다.
     * @note 동시성으로 인해 정확하지 않을 수 있습니다.
     */
    [[nodiscard]] bool IsEmpty() const;

private:
    /** MPSC Queue Node */
    struct SE_CORE_API Node
    {
        UniqueFunction<void()> work;
        std::atomic<Node*> next = nullptr;

        void* operator new(usize size);
        void operator delete(void* ptr);
    };

private:
    // False Sharing 방지를 위해 각 노드 포인터를 독립된 캐시 라인에 배치

    /** 생산자들이 새로운 노드를 추가하는 지점 */
    alignas(SE_CACHE_LINE) std::atomic<Node*> head_node;

    /** 소비자가 노드를 읽어들이는 지점 */
    alignas(SE_CACHE_LINE) Node* tail_node;

    /** 큐의 경계 조건을 처리하기 위한 더미 노드 */
    Node stub_node;
};
} // namespace se
