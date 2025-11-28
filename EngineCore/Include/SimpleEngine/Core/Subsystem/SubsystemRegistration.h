// ReSharper disable CppMemberFunctionMayBeConst
#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Reflection/TypeId.h"


namespace se::core
{
class ISubsystem;

namespace details
{
/**
 * Subsystem 생성 및 의존성 정보를 담는 메타데이터
 */
struct SubsystemMetadata
{
    using SubsystemFactory = Function<std::unique_ptr<ISubsystem>()>;

    SubsystemFactory factory;
    Array<refl::TypeId> dependencies;
};

/**
 * 메타데이터를 설정하기 위한 체이닝 빌더
 */
template <typename Subsystem>
struct SubsystemBuilder
{
    refl::TypeId target_id;

    template <typename... Dependencies>
        requires (!traits::IsAnyOfDecayed<Subsystem, Dependencies...> && (std::derived_from<Dependencies, ISubsystem> && ...))
    SubsystemBuilder& DependsOn()
    {
        (AddDependency<Dependencies>(), ...);
        return *this;
    }

private:
    template <typename Dependency>
    void AddDependency();
};

/**
 * 자동 등록된 서브시스템의 생성자(팩토리)와 메타데이터를 보관하는 레지스트리
 *
 * Engine 초기화 이후 적절한 시점에 SE_REGISTER_SUBSYSTEM에 의해 미리 등록된 Subsystem을 읽어 Engine에 등록합니다.
 */
class SE_CORE_API SubsystemRegistry
{
public:
    static SubsystemRegistry& GetInstance()
    {
        static SubsystemRegistry instance;
        return instance;
    }

    ~SubsystemRegistry() = default;

    SubsystemRegistry(const SubsystemRegistry&) = delete;
    SubsystemRegistry& operator=(const SubsystemRegistry&) = delete;
    SubsystemRegistry(SubsystemRegistry&&) = delete;
    SubsystemRegistry& operator=(SubsystemRegistry&&) = delete;

public:
    /** 서브시스템을 등록합니다. */
    template <typename Subsystem>
        requires std::derived_from<Subsystem, ISubsystem>
    static SubsystemBuilder<Subsystem> Register()
    {
        refl::TypeId id = refl::TypeId::Get<Subsystem>();

        GetInstance().metadata_map.Emplace(id, SubsystemMetadata{
            .factory = [] static -> std::unique_ptr<ISubsystem>
            {
                return std::make_unique<Subsystem>();
            },
            .dependencies = {}
        });

        return SubsystemBuilder<Subsystem>{ id };
    }

    [[nodiscard]] SubsystemMetadata& GetMetadata(const refl::TypeId& id) { return metadata_map.FindChecked(id); }
    [[nodiscard]] const auto& GetMetadataMap() const { return metadata_map; }
    void ClearMetadataMap() { metadata_map.Clear(); }

private:
    SubsystemRegistry() = default;
    HashMap<refl::TypeId, SubsystemMetadata> metadata_map;
};

template <typename Subsystem>
template <typename Dependency>
void SubsystemBuilder<Subsystem>::AddDependency()
{
    SubsystemRegistry::GetInstance()
        .GetMetadata(target_id).dependencies
        .Push(refl::TypeId::Get<Dependency>());
}
}
}

/** 서브시스템 클래스의 .cpp 파일 내에서 이 매크로를 호출하여 해당 서브시스템을 엔진에 자동 등록합니다. */
#define SE_REGISTER_SUBSYSTEM(type) \
    static const auto type##_Registrar = ::se::core::details::SubsystemRegistry::Register<type>()
