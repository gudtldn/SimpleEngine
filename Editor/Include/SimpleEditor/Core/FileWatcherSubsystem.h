#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Core/FileWatcher.h"

#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
/**
 * FileWatcher의 수명 주기를 관리하는 Subsystem
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) FileWatcherSubsystem : public SubsystemBase
{
    SE_CLASS(FileWatcherSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    /**
     * 디렉토리 감시를 등록합니다.
     * @param path 감시할 디렉토리의 절대 경로
     * @param recursive true이면 하위 디렉토리의 변경도 감지합니다.
     * @return 성공 시 유효한 WatchId, 실패 시 WatchId::INVALID
     */
    WatchId Watch(const Path& path, bool recursive = true);

    /**
     * Watch()로 등록한 감시를 해제합니다.
     * @param watch_id Watch()가 반환한 WatchId
     */
    void Unwatch(WatchId watch_id);

    /**
     * 해당 WatchId에 누적된 이벤트들을 빼서 반환합니다.
     * @note MainThread-Only
     * @param watch_id 이벤트를 수거할 WatchId
     * @return 누적된 이벤트 배열 (비어있을 수도 있음)
     */
    [[nodiscard]] Array<FileWatchEvent> DrainEvents(WatchId watch_id);

private:
    FileWatcher watcher;
};
} // namespace se::editor
