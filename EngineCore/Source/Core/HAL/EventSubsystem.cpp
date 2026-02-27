#include "SimpleEngine/Core/HAL/EventSubsystem.h"

#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/Reflect.h"
#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"


namespace se
{
SE_REGISTER_SUBSYSTEM(EventSubsystem);

SE_BEGIN_REFLECT(EventSubsystem, meta::Internal)
SE_END_REFLECT(EventSubsystem)


bool EventSubsystem::Initialize()
{
    ConsoleLog(ELogLevel::Info, "Initializing Event Subsystem...");

    if (!SDL_InitSubSystem(SDL_INIT_EVENTS))
    {
        ConsoleLog(ELogLevel::Error, "SDL_InitSubSystem failed: {}", SDL_GetError());
        return false;
    }

    ConsoleLog(ELogLevel::Info, "Event Subsystem initialized");
    return true;
}

void EventSubsystem::Release()
{
    ConsoleLog(ELogLevel::Info, "Releasing Event Subsystem...");

    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void EventSubsystem::PollEvents() // NOLINT(*-make-member-function-const)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        on_sdl_event.Broadcast(event);

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            on_quit_requested.Broadcast();
            break;

        default:
            break;
        }
    }
}
} // namespace se
