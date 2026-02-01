#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se
{
class TaskScheduler;
class ThreadPool;
}

namespace se
{
class IUpdatable;
class ISubsystem;

/**
 * 엔진의 핵심 기능을 담당하는 클래스입니다.
 * Subsystem의 Register, Initialize, Release 및 Update와 같은 동작을 관리합니다.
 */
class SE_CORE_API Engine
{
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    static Engine& Get();

public:
    /**
     * 전역 레지스트리에 자동 등록된 서브시스템들을 엔진에 로드합니다.
     *
     * 프로그램 시작 시 SE_REGISTER_SUBSYSTEM 매크로에 의해 전역 레지스트리에 등록된
     * 서브시스템 팩토리 함수들을 실행하여 각 서브시스템의 인스턴스를 생성합니다.
     * 이 함수는 엔진 초기화 과정에서 반드시 한 번만 호출되어야 합니다.
     */
    void LoadRegisteredSubsystems();

    /**
     * 등록된 Subsystem을 가져옵니다.
     * @return 등록된 T 타입의 Subsystem 포인터. 없을 경우 nullptr를 반환합니다.
     */
    template <typename T>
        requires std::derived_from<T, ISubsystem>
    [[nodiscard]] T* GetSubsystem() const;

public:
    /** Engine을 초기화 합니다 */
    [[nodiscard]] bool Initialize();

    /** Engine이 가지고 있던 객체를 정리합니다. */
    void Release();

    /** 모든 Subsystem에 대해 위상 정렬된 순서대로 Update을 호출합니다. */
    void UpdateFrame(float delta_time);

private:
    /** 모든 Subsystem을 위상 정렬된 순서대로 초기화 합니다. */
    [[nodiscard]] bool InitializeAllSubsystems();

    /** 모든 Subsystem을 위상 정렬된 순서의 역순으로 정리합니다. */
    void ReleaseAllSubsystems();

    /**
     * 의존성 그래프를 기반으로 서브시스템의 실행 순서를 위상 정렬합니다.
     * 순환 의존성이 발견되면 false를 반환합니다.
     *
     * @see https://en.wikipedia.org/wiki/Topological_sorting
     */
    [[nodiscard]] bool SortSubsystems();

private:
    static Engine* Instance;

    // Type별 Subsystem 목록 | TODO: MSVC flat_map 나오면 수정
    HashMap<TypeId, std::unique_ptr<ISubsystem>> subsystems;

    // 초기화/종료 순서 관리를 위한 벡터
    Array<ISubsystem*> sorted_subsystems;

    // Update가 필요한 Subsystem 목록
    Array<IUpdatable*> updatable_systems;

    std::unique_ptr<TaskScheduler> task_scheduler;
};


template <typename T>
    requires std::derived_from<T, ISubsystem>
T* Engine::GetSubsystem() const
{
    const auto type_id = TypeId::Get<T>();
    if (Optional subsystem = subsystems.Find(type_id))
    {
        return static_cast<T*>(subsystem->get());
    }
    return nullptr;
}
}  // namespace se
