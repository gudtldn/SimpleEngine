#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/Core/FileWatcher.h"

#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"


namespace se::editor
{
/**
 * FileWatcher의 수명 주기를 관리하는 Subsystem
 */
class SE_EDITOR_API SE_ANNOTATION(=meta::Internal) FileWatcherSubsystem : public SubsystemBase
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
     * @param callback 변경 발생 시 호출할 콜백 (백그라운드 스레드에서 호출됨)
     * @param recursive true이면 하위 디렉토리의 변경도 감지합니다.
     * @return 성공 시 Unwatch에 사용할 양의 정수 ID, 실패 시 음수
     */
    WatchId Watch(const Path& path, FileWatchCallback callback, bool recursive = true);

    /**
     * Watch()로 등록한 감시를 해제합니다.
     * @param watch_id Watch()가 반환한 WatchId
     */
    void Unwatch(WatchId watch_id);

private:
    FileWatcher watcher;
};
} // namespace se::editor
