#pragma once

#include "SimpleEditor/Core/FileWatcher.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
/**
 * 셰이더 컴파일 및 핫 리로드를 담당하는 Subsystem
 */
class SE_ANNOTATION(=meta::Internal) ShaderCompileSubsystem : public se::SubsystemBase, public se::IUpdatable
{
    SE_CLASS(ShaderCompileSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void Update(f64 delta_time) override;
    //~ End IUpdatable

private:
    /** 감시 중인 모든 셰이더 디렉토리를 전체 재컴파일합니다. */
    void RecompileAll();

    /** pending_files에 등록된 파일만 선택적으로 재컴파일합니다. */
    void RecompilePending();

private:
    /** 셰이더 디렉토리 정보를 담는 구조체 (소스 디렉토리 + 컴파일 출력 디렉토리) */
    struct ShaderSource
    {
        Path source_dir;
        Path output_dir;
        WatchId watch_id;
    };

    Array<ShaderSource> sources;

    /**
     * 재컴파일 대기 중인 파일 목록
     * key: 변경된 .hlsl 절대 경로 / value: sources 내 인덱스
     */
    HashMap<Path, usize> pending_files;
};
} // namespace se::editor
