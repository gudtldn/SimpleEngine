// ReSharper disable CppDFAMemoryLeak
#include "SimpleEngine/Core/Concurrency/MpscTaskLinkedQueue.h"
#include "SimpleEngine/Core/Concurrency/JobAllocator.h"


namespace se
{
MpscTaskLinkedQueue::MpscTaskLinkedQueue()
{
    // stub.next를 null로 초기화
    stub_node.next.store(nullptr, std::memory_order_relaxed);

    // head와 tail 모두 stub를 가리키도록 초기화
    head_node.store(&stub_node, std::memory_order_relaxed);
    tail_node = &stub_node;
}

MpscTaskLinkedQueue::~MpscTaskLinkedQueue()
{
    // 남은 노드를 모두 해제 (stub는 멤버이므로 delete 대상에서 제외)
    const Node* current = tail_node;
    while (current != nullptr)
    {
        const Node* next = current->next.load(std::memory_order_relaxed);
        if (current != &stub_node)
        {
            delete current;
        }
        current = next;
    }
}

void MpscTaskLinkedQueue::Push(UniqueFunction<void()>&& in_work)
{
    // 새 노드 할당
    Node* node = new Node();
    node->work = std::move(in_work);
    node->next.store(nullptr, std::memory_order_relaxed);

    // head를 원자적으로 새 노드로 교체 (Producer 간의 유일한 경합 지점)
    // acq_rel: 이전 Producer의 쓰기 작업을 보고(acquire), 현재의 쓰기를 다음 Producer/Consumer에게 알림(release)
    Node* prev = head_node.exchange(node, std::memory_order_acq_rel);

    // 이전 노드의 next를 새 노드에 연결
    // 이 시점 전까지는 head가 바뀌었더라도 Consumer는 next가 null이므로 새 노드를 볼 수 없다. (Inconsistency 구간)
    // release: 위에서 설정한 node->work의 이동(move)이 Consumer에게 가시적으로 보임을 보장
    prev->next.store(node, std::memory_order_release);
}

u32 MpscTaskLinkedQueue::Drain()
{
    u32 executed_count = 0;
    Node* tail = tail_node;

    while (true)
    {
        Node* next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr)
        {
            break;
        }

        // 작업 실행
        if (next->work)
        {
            next->work();
            next->work = nullptr;
        }

        // 소모된 이전 노드(tail) 정리
        // Vyukov 큐는 '현재 실행 중인 노드'가 아니라 '그 이전 노드'를 해제하는 방식으로 전진한다.
        if (tail != &stub_node)
        {
            delete tail;
        }

        // 다음 노드로 이동
        tail = next;
        ++executed_count;
    }

    tail_node = tail;
    return executed_count;
}

bool MpscTaskLinkedQueue::IsEmpty() const
{
    // tail의 다음이 없다면 현재 소비할 수 있는 작업이 없는 상태
    return tail_node->next.load(std::memory_order_acquire) == nullptr;
}

void* MpscTaskLinkedQueue::Node::operator new(usize size)
{
    return JobAllocator::Allocate(size);
}

void MpscTaskLinkedQueue::Node::operator delete(void* ptr)
{
    JobAllocator::Free(ptr);
}
} // namespace se
