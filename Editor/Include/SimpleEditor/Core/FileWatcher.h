#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Path.h"

#include <memory>


namespace se::editor
{
/**
 * 파일 시스템에서 발생한 Action의 종류
 */
enum class EFileWatchAction
{
    Added,
    Deleted,
    Modified,
    Moved,
};

/**
 * 파일 시스템 이벤트 하나를 담는 구조체
 */
struct FileWatchEvent
{
    Path directory;          // 변경이 발생한 디렉토리의 경로
    String filename;         // 변경된 파일 또는 디렉토리의 이름 (경로 미포함)
    EFileWatchAction action; // 발생한 변경의 Action

    /** Moved 액션에서만 유효. 이동 또는 이름 변경 이전의 파일 이름을 나타냅니다. */
    String old_filename;
};

/**
 * Watch()가 반환하는 감시 등록 핸들
 */
struct WatchId
{
    static constexpr i32 INVALID = -1;

    i32 value = INVALID;

    [[nodiscard]] bool IsValid() const { return value >= 0; }
    explicit operator bool() const { return IsValid(); }
    bool operator==(const WatchId&) const = default;
};

/**
 * efsw 기반의 파일 시스템 감시 Wrapper
 *
 * Watch()로 하나 이상의 디렉토리를 등록한 뒤 Start()를 호출하면,
 * 내부 백그라운드 스레드가 변경을 감지하여 WatchId별 inbox에 이벤트를 누적합니다.
 * 메인 스레드에서는 DrainEvents()로 누적된 이벤트를 일괄 회수할 수 있습니다.
 *
 * @code
 *   FileWatcher watcher;
 *   const WatchId id = watcher.Watch(asset_dir);
 *   watcher.Start();
 *   // ... 매 프레임:
 *   for (const FileWatchEvent& ev : watcher.DrainEvents(id))
 *   {
 *       // ev.filename, ev.action으로 변경 처리
 *   }
 * @endcode
 */
class SE_EDITOR_API FileWatcher
{
public:
    FileWatcher();
    ~FileWatcher();

    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    /**
     * 디렉토리 감시를 등록합니다.
     * @note Start() 호출 전후 모두 등록 가능합니다.
     * @param path 감시할 디렉토리의 절대 경로
     * @param recursive true이면 하위 디렉토리의 변경도 감지합니다.
     * @return 등록 성공 시 유효한 WatchId, 실패 시 WatchId::INVALID
     */
    WatchId Watch(const Path& path, bool recursive = true);

    /**
     * Watch()로 등록한 감시를 해제합니다.
     * @param watch_id Watch()가 반환한 WatchId
     */
    void Unwatch(WatchId watch_id);

    /**
     * 백그라운드 감시 스레드를 시작합니다.
     * @note Watch()로 디렉토리를 하나 이상 등록한 뒤 호출해야 합니다. 중복 호출은 무시됩니다.
     */
    void Start();

    /**
     * 해당 WatchId에 누적된 이벤트들을 빼서 반환합니다.
     * 반환 후 내부 inbox는 비워지며, 다음 Drain까지 새 이벤트가 다시 누적됩니다.
     * @note MainThread-Only
     * @param watch_id 이벤트를 수거할 WatchId
     * @return 누적된 이벤트 배열 (비어있을 수도 있음)
     */
    [[nodiscard]] Array<FileWatchEvent> DrainEvents(WatchId watch_id);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace se::editor
