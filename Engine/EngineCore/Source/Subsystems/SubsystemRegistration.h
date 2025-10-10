#pragma once
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>


/**
 * 서브시스템 클래스 내에서 이 매크로를 호출하여 해당 서브시스템을 엔진에 자동 등록합니다.
 * @todo 나중에 Engine을 dynamic lib로 변경하면 그때 적용
 */
#define SE_REGISTER_SUBSYSTEM(subsystem) \
    inline static struct subsystem##Registerer \
    { \
        subsystem##Registerer() \
        { \
            using se::subsystem_register::details::SubsystemRegistry; \
            SubsystemRegistry::GetSubsystemRegistry()[std::type_index(typeid(subsystem))] = [] -> void* \
            { \
                return ::new (subsystem); \
            }; \
            if constexpr (std::derived_from<subsystem, IUpdatable>) \
            { \
                SubsystemRegistry::GetUpdatableSystems().emplace_back(typeid(subsystem)); \
            } \
        } \
    } subsystem##Registerer_PRIVATE{};


namespace se::subsystem_register::details
{
/**
 * 자동 등록된 서브시스템 정보를 임시로 보관하는 전역 레지스트리
 */
struct SubsystemRegistry
{
    static auto& GetSubsystemRegistry()
    {
        static std::unordered_map<std::type_index, std::function<void*()>> registry;
        return registry;
    }

    static auto& GetUpdatableSystems()
    {
        static std::vector<std::type_index> updatable_systems;
        return updatable_systems;
    }
};
}
