module;
#include <typeinfo>
module SimpleEngine.Engine;

import SimpleEngine.Utils;
import std;


bool Engine::Initialize()
{
    return true;
}

void Engine::Release()
{
}

bool Engine::InitializeAllSubSystems()
{
    ConsoleLog(ELogLevel::Info, u8"Initializing SubSystems...");
    for (ISubSystem* sub_system : sub_systems_list)
    {
        if (!sub_system->Initialize())
        {
            const std::u8string sub_system_name = se::string_utils::ToU8String(typeid(sub_system).name());
            ConsoleLog(ELogLevel::Error, u8"SubSystem {} failed to initialize!", sub_system_name);

            const auto current_it = std::ranges::find(sub_systems_list, sub_system);
            const auto subrange = std::ranges::subrange(sub_systems_list.begin(), current_it);
            for (ISubSystem* rev_subsystem : subrange | std::views::reverse)
            {
                rev_subsystem->Release();
            }
            return false;
        }
    }
    ConsoleLog(ELogLevel::Info, u8"All SubSystems initialized successfully");
    return true;
}

void Engine::ReleaseAllSubSystems()
{
    ConsoleLog(ELogLevel::Info, u8"Releasing SubSystems...");
    for (ISubSystem* sub_system : sub_systems_list | std::views::reverse)
    {
        sub_system->Release();
    }
    ConsoleLog(ELogLevel::Info, u8"All SubSystems released successfully");
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Engine::TickAllSubSystems(float delta_time)
{
    for (ISubSystem* sub_system : sub_systems_list)
    {
        sub_system->Tick(delta_time);
    }
}
