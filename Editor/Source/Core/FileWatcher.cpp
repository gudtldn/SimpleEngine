#include "SimpleEditor/Core/FileWatcher.h"

#include "SimpleEngine/Core/Container/HashMap.h"

#include "efsw/efsw.hpp"

#include <mutex>


namespace se::editor
{
struct FileWatcher::Impl final : public efsw::FileWatchListener
{
    efsw::FileWatcher watcher;
    TracyLockable(std::mutex, mutex);
    HashMap<i32, Array<FileWatchEvent>> inboxes; // key: WatchId.value

    virtual void handleFileAction(
        efsw::WatchID id,
        const std::string& dir,
        const std::string& filename,
        efsw::Action action,
        std::string old_filename
    ) override
    {
        FileWatchEvent event;
        event.directory = dir.c_str();
        event.filename = filename.c_str();
        event.old_filename = old_filename.c_str();

        switch (action)
        {
        case efsw::Actions::Add:
        {
            event.action = EFileWatchAction::Added;
            break;
        }
        case efsw::Actions::Delete:
        {
            event.action = EFileWatchAction::Deleted;
            break;
        }
        case efsw::Actions::Modified:
        {
            event.action = EFileWatchAction::Modified;
            break;
        }
        case efsw::Actions::Moved:
        {
            event.action = EFileWatchAction::Moved;
            break;
        }
        }

        std::scoped_lock lock{ mutex };
        if (const auto found = inboxes.Find(static_cast<i32>(id)))
        {
            found->Push(std::move(event));
        }
    }
};

FileWatcher::FileWatcher()
    : impl(std::make_unique<Impl>())
{
}

FileWatcher::~FileWatcher() = default;

WatchId FileWatcher::Watch(const Path& path, bool recursive)
{
    const auto efsw_id = impl->watcher.addWatch(path.CStr(), impl.get(), recursive);
    const WatchId id{ .value = static_cast<i32>(efsw_id) };

    if (id)
    {
        std::scoped_lock lock{ impl->mutex };
        impl->inboxes.Insert(id.value, Array<FileWatchEvent>{});
    }
    return id;
}

void FileWatcher::Unwatch(WatchId watch_id)
{
    impl->watcher.removeWatch(static_cast<efsw::WatchID>(watch_id.value));

    std::scoped_lock lock{ impl->mutex };
    impl->inboxes.Remove(watch_id.value);
}

void FileWatcher::Start()
{
    impl->watcher.watch();
}

Array<FileWatchEvent> FileWatcher::DrainEvents(WatchId watch_id)
{
    Array<FileWatchEvent> result;

    std::scoped_lock lock{ impl->mutex };
    if (const auto found = impl->inboxes.Find(watch_id.value))
    {
        result.Swap(*found);
    }
    return result;
}
} // namespace se::editor