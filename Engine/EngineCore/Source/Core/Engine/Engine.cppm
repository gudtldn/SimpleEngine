export module SE.Core:Engine;
import :Logging;
import :Reflection;
import :Concurrency;

import SE.Interface.ISubsystemBase;
import SE.Interface.IUpdatable;
import std;


namespace se::core::engine
{
/**
 * 엔진의 핵심 기능을 담당하는 클래스입니다.
 * Subsystem의 Register, Initialize, Release 및 Update와 같은 동작을 관리합니다.
 */
export class Engine
{
public:
    Engine() = default;
    ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

public:
    /**
     * Subsystem을 엔진에 등록합니다.
     *
     * @tparam T 등록할 서브시스템의 타입입니다. 반드시 ISubsystem을 상속받아야 합니다.
     * @param args Subsystem의 생성자에 전달될 인자들
     * @return 새로 생성된 T 타입의 서브시스템 포인터, 또는 이미 등록된 경우 해당 서브시스템의 포인터를 반환합니다.
     */
    template <typename T, typename... Args>
        requires std::derived_from<T, ISubsystemBase>
    T* RegisterSubsystem(Args&&... args);

    /**
     * 등록된 Subsystem을 가져옵니다.
     * @return 등록된 T 타입의 Subsystem 포인터. 없을 경우 nullptr를 반환합니다.
     */
    template <typename T>
        requires std::derived_from<T, ISubsystemBase>
    [[nodiscard]] T* GetSubsystem() const;

    /** Thread Pool을 가져옵니다. */
    concurrency::ThreadPool& GetThreadPool() const { return *thread_pool; }

    /** Task Scheduler를 가져옵니다 */
    concurrency::TaskScheduler& GetTaskScheduler() const { return *task_scheduler; }

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
    // Type별 Subsystem 목록 | TODO: MSVC flat_map 나오면 수정
    unordered_map<std::type_index, std::unique_ptr<ISubsystemBase>> subsystems;

    // 초기화/종료 순서 관리를 위한 벡터
    vector<ISubsystemBase*> sorted_subsystems;

    // Update가 필요한 Subsystem 목록
    vector<IUpdatable*> updatable_systems;

    // Engine에서 사용할 ThreadPool과 TaskScheduler
    std::unique_ptr<concurrency::ThreadPool> thread_pool;
    std::unique_ptr<concurrency::TaskScheduler> task_scheduler;
};


template <typename T, typename... Args> requires std::derived_from<T, ISubsystemBase>
T* Engine::RegisterSubsystem(Args&&... args)
{
    const auto type_id = std::type_index(typeid(T));
    if (subsystems.contains(type_id))
    {
        return static_cast<T*>(subsystems[type_id].get());
    }

    auto sub_system = std::make_unique<T>(std::forward<Args>(args)...);
    T* sub_system_ptr = sub_system.get();

    subsystems[type_id] = std::move(sub_system);

    if constexpr (std::derived_from<T, IUpdatable>)
    {
        updatable_systems.push_back(static_cast<IUpdatable*>(sub_system_ptr));
    }

    ConsoleLog(ELogLevel::Debug, u8"Registered Subsystem: {}", reflection::GetTypeSignature<T>());
    return sub_system_ptr;
}

template <typename T> requires std::derived_from<T, ISubsystemBase>
T* Engine::GetSubsystem() const
{
    const auto type_id = std::type_index(typeid(T));
    if (subsystems.contains(type_id))
    {
        return static_cast<T*>(subsystems.at(type_id).get());
    }
    return nullptr;
}
}
