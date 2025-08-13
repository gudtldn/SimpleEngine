export module SimpleEngine.Core:Logging.Backends.FileBackend;
import :Logging.Backends.ILogBackend;

import SimpleEngine.Types;
import std;


namespace se::core::logging::backends
{
export class FileBackend : public ILogBackend
{
public:
    FileBackend();
    FileBackend(std::filesystem::path path);

    virtual void WriteLog(const LogEntry& entry) override;
    virtual void Flush() override;

private:
    /** .log 파일을 새로 엽니다. (없다면 새로 생성) */
    void OpenFile();

    /** 현재 로그 파일을 백업하고 새로운 파일을 생성합니다. */
    void RotateFile();

    /** 현재 파일 크기가 max_file_size를 초과했는지 확인합니다. */
    [[nodiscard]] bool CheckRotation() const;

private:
    std::ofstream file;
    std::filesystem::path file_path;
    size_t current_file_size = 0;
    constexpr static size_t max_file_size = 1024 * 1024 * 10;
};
}
