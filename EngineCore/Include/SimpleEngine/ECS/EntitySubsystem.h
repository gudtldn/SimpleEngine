#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/ECS/WorldContext.h"


namespace se
{
/**
 * ECS WorldContext 컬렉션을 관리하고 엔진 업데이트 루프와 연결하는 Subsystem
 */
class SE_CORE_API SE_ANNOTATION(=meta::Internal) EntitySubsystem : public SubsystemBase, public IUpdatable
{
    SE_CLASS(EntitySubsystem, SubsystemBase)

public:
    EntitySubsystem() = default;
    virtual ~EntitySubsystem() override = default;

    EntitySubsystem(const EntitySubsystem&) = delete;
    EntitySubsystem& operator=(const EntitySubsystem&) = delete;
    EntitySubsystem(EntitySubsystem&&) = default;
    EntitySubsystem& operator=(EntitySubsystem&&) = default;

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void Update(f64 delta_time) override;
    //~ End IUpdatable

public:
    /** World를 가져옵니다. 존재하지 않으면 새로 생성합니다. */
    WorldContext& GetOrCreateWorld(const StringName& name);

    /** 이름으로 WorldContext를 찾습니다. 존재하지 않으면 NullOpt를 반환합니다. */
    [[nodiscard]] Optional<WorldContext&> FindWorld(const StringName& name);

    /** Main World를 반환합니다. */
    [[nodiscard]] WorldContext& GetMainWorld();
    [[nodiscard]] const WorldContext& GetMainWorld() const;

    /** World를 제거합니다. */
    void DestroyWorld(const StringName& name);

    /** 등록된 모든 WorldContext에 접근합니다. */
    [[nodiscard]] HashMap<StringName, WorldContext>& GetWorlds();
    [[nodiscard]] const HashMap<StringName, WorldContext>& GetWorlds() const;

private:
    static const StringName& GetMainWorldName();

private:
    HashMap<StringName, WorldContext> worlds;
};
} // namespace se
