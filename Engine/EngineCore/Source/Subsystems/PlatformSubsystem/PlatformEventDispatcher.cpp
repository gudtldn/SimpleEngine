module SimpleEngine.Subsystems.PlatformSubsystem;
import :PlatformEventDispatcher;


SubscriptionHandle EventDispatcher::Subscribe(EventPriority priority, EventCallback callback)
{
    const SubscriptionHandle handle = SubscriptionHandle::CreateHandle();
    PriorityMap[priority].push_back(handle);
    Subscriptions[handle] = {
        .Priority = priority,
        .Callback = std::move(callback)
    };
    return handle;
}

void EventDispatcher::Unsubscribe(SubscriptionHandle handle)
{
    // 유효하지 않은 Handle이면 return
    if (!handle.IsValid() || !Subscriptions.contains(handle))
    {
        return;
    }

    const EventPriority priority = Subscriptions[handle].Priority;

    Subscriptions.erase(handle);
    if (PriorityMap.contains(priority))
    {
        // 나중에 swap_remove 고민해보기
        auto& handle_vector = PriorityMap[priority];
        std::erase(handle_vector, handle);
    }
}

void EventDispatcher::Dispatch(PlatformEvent& event)
{
    for (const auto& handle_vector : PriorityMap | std::views::values)
    {
        for (const auto& handle : handle_vector)
        {
            // Unsubscribe 되었지만 아직 PriorityMap에서 제거되지 않은 경우를 대비
            if (Subscriptions.contains(handle))
            {
                Subscriptions[handle].Callback(event);

                if (event.Handled)
                {
                    return;
                }
            }
        }
    }
}
