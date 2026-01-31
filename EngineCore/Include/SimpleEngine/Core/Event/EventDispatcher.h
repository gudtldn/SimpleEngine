#pragma once
#include <atomic>
#include <functional>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Map.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

#include "SDL3/SDL.h"

// forward declaration
namespace se::event
{
class SubscriptionHandle;
}


// Specialization for SubscriptionHandle
template <>
struct std::hash<se::event::SubscriptionHandle>
{
    size_t operator()(const se::event::SubscriptionHandle& handle) const noexcept;
};

namespace se::event
{
class SE_CORE_API SubscriptionHandle
{
    uint64 handle_id;

    explicit SubscriptionHandle();
    explicit SubscriptionHandle(uint64 in_handle_id);

    static uint64 GenerateNewID();

public:
    static SubscriptionHandle CreateHandle();

    [[nodiscard]] FORCE_INLINE uint64 GetID() const { return handle_id; }
    [[nodiscard]] FORCE_INLINE bool IsValid() const { return handle_id != 0; }
    FORCE_INLINE void Invalidate() { handle_id = 0; }

    [[nodiscard]] bool operator==(const SubscriptionHandle& other) const = default;
    [[nodiscard]] auto operator<=>(const SubscriptionHandle&) const = default;
};


enum class EventPriority : uint8
{
    High   = 0, // UI 등 이벤트를 선점해야 하는 경우
    Normal = 1, // 일반적인 게임 로직
    Low    = 2  // 로깅 등 가장 나중에 처리되어도 되는 경우
};

struct PlatformEvent
{
    SDL_Event& sdl_event;
    bool handled;
};

class SE_CORE_API EventDispatcher
{
public:
    using EventCallback = Function<void(PlatformEvent&)>;

    SubscriptionHandle Subscribe(EventPriority priority, EventCallback callback);
    void Unsubscribe(SubscriptionHandle handle);

    void Dispatch(PlatformEvent& event);

private:
    struct Subscription
    {
        EventPriority priority;
        EventCallback callback;
    };

    HashMap<SubscriptionHandle, Subscription> subscriptions;
    Map<EventPriority, Array<SubscriptionHandle>> priority_map;
};
}
