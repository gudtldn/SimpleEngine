export module SimpleEngine.Subsystems.PlatformSubsystem:PlatformEventDispatcher;

import SimpleEngine.Platform.Types;
import SimpleEngine.Core.Function;
import std;
import <SDL3/SDL.h>;


export class SubscriptionHandle
{
    friend struct std::hash<SubscriptionHandle>;

    uint64 HandleId;

    explicit SubscriptionHandle()
        : HandleId(0)
    {
    }

    explicit SubscriptionHandle(uint64 HandleId)
        : HandleId(HandleId)
    {
    }

    static uint64 GenerateNewID()
    {
        static std::atomic<uint64> NextHandleId = 1;
        uint64 Result = NextHandleId.fetch_add(1, std::memory_order_relaxed);

        // Overflow가 발생했다면
        if (Result == 0)
        {
            // 한번 더 더하기
            Result = NextHandleId.fetch_add(1, std::memory_order_relaxed);
        }

        return Result;
    }

public:
    static SubscriptionHandle CreateHandle()
    {
        return SubscriptionHandle{GenerateNewID()};
    }

    bool IsValid() const { return HandleId != 0; }
    void Invalidate() { HandleId = 0; }

    std::strong_ordering operator<=>(const SubscriptionHandle&) const = default;
};

template <>
struct std::hash<SubscriptionHandle>
{
    size_t operator()(const SubscriptionHandle& handle) const noexcept
    {
        return std::hash<uint64>{}(handle.HandleId);
    }
};


export enum class EventPriority : uint8
{
    High   = 0, // UI 등 이벤트를 선점해야 하는 경우
    Normal = 1, // 일반적인 게임 로직
    Low    = 2  // 로깅 등 가장 나중에 처리되어도 되는 경우
};

export struct PlatformEvent
{
    SDL_Event& SdlEvent;
    bool Handled;
};

export class EventDispatcher
{
public:
    using EventCallback = Function<void(PlatformEvent&)>;

    SubscriptionHandle Subscribe(EventPriority priority, EventCallback callback);
    void Unsubscribe(SubscriptionHandle handle);

    void Dispatch(PlatformEvent& event);

private:
    struct Subscription
    {
        EventPriority Priority;
        EventCallback Callback;
    };

    std::unordered_map<SubscriptionHandle, Subscription> Subscriptions;
    std::map<EventPriority, std::vector<SubscriptionHandle>> PriorityMap;
};
