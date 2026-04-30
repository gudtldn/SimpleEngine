#pragma once
#include <concepts>
#include <memory>
#include <mutex>

#include "SimpleEngine/Core/Container/FlatMap.h"
#include "SimpleEngine/Core/Logging/LogData.h"
#include "SimpleEngine/Core/Logging/Backends/ILogBackend.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"


namespace se
{
class SE_CORE_API LogBackendManager
{
private:
    LogBackendManager() = default;

public:
    ~LogBackendManager() = default;

    LogBackendManager(const LogBackendManager&) = delete;
    LogBackendManager& operator=(const LogBackendManager&) = delete;
    LogBackendManager(LogBackendManager&&) = delete;
    LogBackendManager& operator=(LogBackendManager&&) = delete;

    static LogBackendManager& Get()
    {
        static LogBackendManager log_backend_manager;
        return log_backend_manager;
    }

public:
    template <typename T, typename... Args>
        requires std::derived_from<T, ILogBackend>
    void AddBackend(Args&&... args)
    {
        const auto type_id = TypeId::Get<T>();

        std::scoped_lock lock(backends_mutex);
        backends.Insert(type_id, std::make_unique<T>(std::forward<Args>(args)...));
    }

    template <typename T>
        requires std::derived_from<T, ILogBackend>
    [[nodiscard]] T* GetBackend() const
    {
        const auto type_id = TypeId::Get<T>();
        constexpr std::unique_ptr<ILogBackend> null_ptr;

        std::scoped_lock lock(backends_mutex);
        return static_cast<T*>(backends.Find(type_id).ValueOr(null_ptr).get());
    }

    void WriteToAllBackends(const LogEntry& entry);
    void FlushAllBackends();

private:
    mutable std::mutex backends_mutex;
    FlatMap<TypeId, std::unique_ptr<ILogBackend>> backends{};
};
} // namespace se
