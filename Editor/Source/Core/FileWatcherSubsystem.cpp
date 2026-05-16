#include "SimpleEditor/Core/FileWatcherSubsystem.h"

#include "SimpleEngine/Core/Subsystem/SubsystemRegistration.h"


namespace se::editor
{
SE_REGISTER_SUBSYSTEM(FileWatcherSubsystem);

SE_BEGIN_REFLECT(FileWatcherSubsystem, meta::Internal)
SE_END_REFLECT(FileWatcherSubsystem)

bool FileWatcherSubsystem::Initialize()
{
    watcher.Start();
    return true;
}

void FileWatcherSubsystem::Release()
{
}

WatchId FileWatcherSubsystem::Watch(const Path& path, FileWatchCallback callback, bool recursive)
{
    return watcher.Watch(path, std::move(callback), recursive);
}

void FileWatcherSubsystem::Unwatch(WatchId watch_id)
{
    watcher.Unwatch(watch_id);
}
} // namespace se::editor
