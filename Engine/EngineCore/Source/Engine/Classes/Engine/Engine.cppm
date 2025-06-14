export module SimpleEngine.Engine;

import SimpleEngine.Logging;
import SimpleEngine.Core.ISubSystem;
import std;


/**
 * 엔진의 핵심 기능을 담당하는 클래스입니다.
 * SubSystem의 Register, Initialize, Release 및 Update와 같은 동작을 관리합니다.
 */
export class Engine
{
private:
    // Type별 SubSystem 목록
    std::map<std::type_index, std::unique_ptr<ISubSystem>> sub_systems;

    // 초기화/종료 순서 관리를 위한 벡터
    std::vector<ISubSystem*> sorted_sub_systems;

public:
    Engine() = default;
    virtual ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

public:
    /**
     * SubSystem을 엔진에 등록합니다.
     *
     * @tparam T 등록할 서브시스템의 타입입니다. 반드시 ISubSystem을 상속받아야 합니다.
     * @param args SubSystem의 생성자에 전달될 인자들
     * @return 새로 생성된 T 타입의 서브시스템 포인터, 또는 이미 등록된 경우 해당 서브시스템의 포인터를 반환합니다.
     */
    template <typename T, typename... Args>
        requires std::derived_from<T, ISubSystem>
    T* RegisterSubSystem(Args&&... args)
    {
        const auto type_id = std::type_index(typeid(T));
        if (sub_systems.contains(type_id))
        {
            return static_cast<T*>(sub_systems[type_id].get());
        }

        auto sub_system = std::make_unique<T>(std::forward<Args>(args)...);
        T* sub_system_ptr = sub_system.get();

        sub_systems[type_id] = std::move(sub_system);

        ConsoleLog(ELogLevel::Info, u8"Registered SubSystem: {}", type_id.name());
        return sub_system_ptr;
    }

    /**
     * 등록된 SubSystem을 가져옵니다.
     * @return 등록된 T 타입의 SubSystem 포인터. 없을 경우 nullptr를 반환합니다.
     */
    template <typename T>
        requires std::derived_from<T, ISubSystem>
    T* GetSubSystem() const
    {
        const auto type_id = std::type_index(typeid(T));
        if (const auto it = sub_systems.find(type_id); it != sub_systems.end())
        {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

public:
    /** Engine을 초기화 합니다 */
    [[nodiscard]] bool Initialize();

    /** Engine이 가지고 있던 객체를 정리합니다. */
    void Release();

    /** 모든 SubSystem에 대해 위상 정렬된 순서대로 Update을 호출합니다. */
    void UpdateAllSubSystems(float delta_time);

private:
    /** 모든 SubSystem을 위상 정렬된 순서대로 초기화 합니다. */
    [[nodiscard]] bool InitializeAllSubSystems();

    /** 모든 SubSystem을 위상 정렬된 순서의 역순으로 정리합니다. */
    void ReleaseAllSubSystems();

    /**
     * 의존성 그래프를 기반으로 서브시스템의 실행 순서를 위상 정렬합니다.
     * 순환 의존성이 발견되면 false를 반환합니다.
     *
     * @see https://en.wikipedia.org/wiki/Topological_sorting
     */
    [[nodiscard]] bool SortSubSystems();
};
