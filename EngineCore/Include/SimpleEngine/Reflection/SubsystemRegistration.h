#pragma once
#include <concepts>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Interfaces/ISubsystemBase.h"
#include "SimpleEngine/Core/Interfaces/IUpdatable.h"


namespace se::core
{
class Engine;

namespace details
{
/**
 * 자동 등록된 서브시스템의 생성자(팩토리)와 메타데이터를 보관하는 레지스트리
 *
 * Engine 초기화 이후 적절한 시점에 SE_REGISTER_SUBSYSTEM에 의해 미리 등록된 Subsystem을 읽어 Engine에 등록합니다.
 */
class SE_CORE_API SubsystemRegistry
{
    friend class se::core::Engine;

private:
    SubsystemRegistry() = default;

public:
    ~SubsystemRegistry() = default;

    SubsystemRegistry(const SubsystemRegistry&) = delete;
    SubsystemRegistry& operator=(const SubsystemRegistry&) = delete;
    SubsystemRegistry(SubsystemRegistry&&) = delete;
    SubsystemRegistry& operator=(SubsystemRegistry&&) = delete;

public:
    using SubsystemFactory = Function<std::unique_ptr<ISubsystemBase>()>;

    /** 서브시스템을 등록합니다. */
    template <typename Subsystem>
        requires std::derived_from<Subsystem, ISubsystemBase>
    static void Register()
    {
        const auto type_idx = refl::TypeId::Get<Subsystem>();
        GetInstance().factories[type_idx] = {
            .factory = [] static -> std::unique_ptr<ISubsystemBase>
            {
                return std::make_unique<Subsystem>();
            },
            .is_updatable = std::derived_from<Subsystem, IUpdatable>
        };
    }

private:
    struct SubsystemMetadata
    {
        SubsystemFactory factory;
        bool is_updatable;
    };

    // 싱글톤 인스턴스에 접근
    static SubsystemRegistry& GetInstance()
    {
        static SubsystemRegistry instance;
        return instance;
    }

private:
    HashMap<refl::TypeId, SubsystemMetadata> factories;
};
}
}


/**
 * 서브시스템 클래스 내에서 이 매크로를 호출하여 해당 서브시스템을 엔진에 자동 등록합니다.
 */
#define SE_REGISTER_SUBSYSTEM(subsystem_class) \
    inline static struct subsystem_class##Registrar \
    { \
        subsystem_class##Registrar() \
        { \
            ::se::core::details::SubsystemRegistry::Register<subsystem_class>(); \
        } \
    } subsystem_class##Registrar_PRIVATE{};
