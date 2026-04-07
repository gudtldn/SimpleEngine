// ReSharper disable CppMemberFunctionMayBeConst
#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Functional/Function.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se
{
class SubsystemBase;

namespace detail
{
/**
 * Subsystem 생성 및 의존성 정보를 담는 메타데이터
 */
struct SubsystemMetadata
{
    using SubsystemFactory = Function<std::unique_ptr<SubsystemBase>()>;

    SubsystemFactory factory;

    // 다른 Subsystem간 초기화 순서 의존성 목록
    Array<TypeId> dependencies;

    // IUpdatable 간 업데이트 실행 순서를 위한 의존성 목록
    Array<TypeId> update_dependencies;
};

/**
 * 메타데이터를 설정하기 위한 체이닝 빌더
 */
template <typename Subsystem>
struct SubsystemBuilder
{
    TypeId target_id;

    template <typename... Dependencies>
        requires (!traits::IsAnyOfDecayed<Subsystem, Dependencies...> && (std::derived_from<Dependencies, SubsystemBase> && ...))
    SubsystemBuilder& DependsOn()
    {
        (AddDependency<Dependencies>(), ...);
        return *this;
    }

    /**
     * IUpdatable 간 업데이트 실행 순서를 지정합니다.
     * 지정된 서브시스템들이 이 서브시스템보다 먼저 업데이트됩니다.
     * 미지정 시 다른 IUpdatable과의 업데이트 순서가 보장되지 않습니다.
     */
    template <typename... Dependencies>
        requires (!traits::IsAnyOfDecayed<Subsystem, Dependencies...> && (std::derived_from<Dependencies, SubsystemBase> && ...))
    SubsystemBuilder& UpdateDependsOn()
    {
        (AddUpdateDependency<Dependencies>(), ...);
        return *this;
    }

private:
    template <typename Dependency>
    void AddDependency();

    template <typename Dependency>
    void AddUpdateDependency();
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
        requires std::derived_from<Subsystem, SubsystemBase>
    static SubsystemBuilder<Subsystem> Register()
    {
        TypeId id = TypeId::Get<Subsystem>();

        GetInstance().metadata_map.Emplace(id, SubsystemMetadata{
            .factory = [] static -> std::unique_ptr<SubsystemBase>
            {
                return std::make_unique<Subsystem>();
            },
            .dependencies = {}
        });

        return SubsystemBuilder<Subsystem>{ id };
    }

    [[nodiscard]] SubsystemMetadata& GetMetadata(const TypeId& id) { return metadata_map.FindChecked(id); }
    [[nodiscard]] const auto& GetMetadataMap() const { return metadata_map; }

private:
    SubsystemRegistry() = default;
    HashMap<TypeId, SubsystemMetadata> metadata_map;
};

template <typename Subsystem>
template <typename Dependency>
void SubsystemBuilder<Subsystem>::AddDependency()
{
    SubsystemRegistry::GetInstance()
        .GetMetadata(target_id).dependencies
        .Push(TypeId::Get<Dependency>());
}

template <typename Subsystem>
template <typename Dependency>
void SubsystemBuilder<Subsystem>::AddUpdateDependency()
{
    SubsystemRegistry::GetInstance()
        .GetMetadata(target_id).update_dependencies
        .Push(TypeId::Get<Dependency>());
}
}  // namespace detail
}  // namespace se

/** 서브시스템 클래스의 .cpp 파일 내에서 이 매크로를 호출하여 해당 서브시스템을 엔진에 자동 등록합니다. */
#define SE_REGISTER_SUBSYSTEM(type) \
    static const auto type##_Registrar = ::se::detail::SubsystemRegistry::Register<type>()
