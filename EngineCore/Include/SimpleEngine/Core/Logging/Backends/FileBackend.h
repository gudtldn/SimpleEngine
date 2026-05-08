#pragma once

#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Logging/Backends/ILogBackend.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "SDL3/SDL.h"


namespace se
{
class SE_CORE_API FileBackend : public ILogBackend
{
public:
    FileBackend();
    FileBackend(Path path);
    virtual ~FileBackend() override;

    FileBackend(const FileBackend&) = delete;
    FileBackend& operator=(const FileBackend&) = delete;

    virtual void WriteLog(const LogEntry& entry) override;
    virtual void Flush() override;

private:
    /** .log 파일을 새로 엽니다. (없다면 새로 생성) */
    void OpenFile();

    /** 현재 로그 파일을 닫습니다. */
    void CloseFile();

    /** 현재 로그 파일을 백업하고 새로운 파일을 생성합니다. */
    void RotateFile();

    /** 현재 파일 크기가 max_file_size를 초과했는지 확인합니다. */
    [[nodiscard]] bool CheckRotation() const;

private:
    SDL_IOStream* io_stream = nullptr;
    Path file_path;
    usize current_file_size = 0;
    constexpr static usize max_file_size = 2ULL * 1024 * 1024; // 2MB
};
} // namespace se
