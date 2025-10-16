#include "Core/Event/EventDispatcher.h"
#include <ranges>


namespace se::core::event
{
SubscriptionHandle::SubscriptionHandle()
    : handle_id(0)
{
}

SubscriptionHandle::SubscriptionHandle(uint64 in_handle_id)
    : handle_id(in_handle_id)
{
}

uint64 SubscriptionHandle::GenerateNewID()
{
    static std::atomic<uint64> next_handle_id = 1;
    uint64 result = next_handle_id.fetch_add(1, std::memory_order_relaxed);

    // Overflow가 발생했다면
    if (result == 0)
    {
        // 한번 더 더하기
        result = next_handle_id.fetch_add(1, std::memory_order_relaxed);
    }

    return result;
}

SubscriptionHandle SubscriptionHandle::CreateHandle()
{
    return SubscriptionHandle{ GenerateNewID() };
}

SubscriptionHandle EventDispatcher::Subscribe(EventPriority priority, EventCallback callback)
{
    const SubscriptionHandle handle = SubscriptionHandle::CreateHandle();
    priority_map[priority].push_back(handle);
    subscriptions[handle] = {
        .priority = priority,
        .callback = std::move(callback)
    };
    return handle;
}

void EventDispatcher::Unsubscribe(SubscriptionHandle handle)
{
    // 유효하지 않은 Handle이면 return
    if (!handle.IsValid() || !subscriptions.contains(handle))
    {
        return;
    }

    const EventPriority priority = subscriptions[handle].priority;

    subscriptions.erase(handle);
    if (priority_map.contains(priority))
    {
        // 나중에 swap_remove 고민해보기
        auto& handle_vector = priority_map[priority];
        std::erase(handle_vector, handle);
    }
}

void EventDispatcher::Dispatch(PlatformEvent& event)
{
    for (const auto& handle_vector : priority_map | std::views::values)
    {
        for (const auto& handle : handle_vector)
        {
            // Unsubscribe 되었지만 아직 PriorityMap에서 제거되지 않은 경우를 대비
            if (subscriptions.contains(handle))
            {
                subscriptions[handle].callback(event);

                if (event.handled)
                {
                    return;
                }
            }
        }
    }
}
}

// Specialization for SubscriptionHandle
using se::core::event::SubscriptionHandle;

size_t std::hash<SubscriptionHandle>::operator()(const SubscriptionHandle& handle) const noexcept
{
    return std::hash<uint64>{}(handle.GetID());
}

