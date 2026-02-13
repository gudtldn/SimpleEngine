#pragma once
#include <expected>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se
{
class FileReadError : public IError
{
public:
    enum class EType
    {
        FileNotFound,     // 파일이 존재하지 않음
        FileOpenFailed,   // 파일 열기 실패
        PermissionDenied, // 권한 문제
        InvalidFormat,    // 파일/데이터 포맷 이상
        ReadFailed,       // 읽기 실패
        WriteFailed,      // 쓰기 실패
        UnexpectedEOF,    // 끝까지 읽지 못함, 예기치 않은 EOF
        OutOfMemory,      // 메모리 부족
        UnknownError      // 알 수 없는 에러
    };

    static FileReadError NotFound(String&& msg) { return { EType::FileNotFound, std::move(msg) }; }
    static FileReadError OpenFailed(String&& msg) { return { EType::FileOpenFailed, std::move(msg) }; }
    static FileReadError Permission(String&& msg) { return { EType::PermissionDenied, std::move(msg) }; }
    static FileReadError Format(String&& msg) { return { EType::InvalidFormat, std::move(msg) }; }
    static FileReadError Read(String&& msg) { return { EType::ReadFailed, std::move(msg) }; }
    static FileReadError Write(String&& msg) { return { EType::WriteFailed, std::move(msg) }; }
    static FileReadError EndOfFile(String&& msg) { return { EType::UnexpectedEOF, std::move(msg) }; }
    static FileReadError OutOfMem(String&& msg) { return { EType::OutOfMemory, std::move(msg) }; }
    static FileReadError Unknown(String&& msg) { return { EType::UnknownError, std::move(msg) }; }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] EType GetType() const noexcept { return type; }

private:
    FileReadError(EType in_type, String&& message)
        : type(in_type), message(std::move(message)) {}

    EType type;
    String message;
};

template <typename T>
using FileResult = Expected<T, FileReadError>;

/**
 * 파일 I/O 관련 유틸리티 함수 모음
 */
struct SE_CORE_API FileIO
{
    FileIO() = delete;

    /** 파일을 읽고, byte array로 반환합니다. */
    [[nodiscard]] static FileResult<Array<uint8>> ReadBytes(const Path& file_path);

    /** 파일을 읽고, string으로 반환합니다. */
    [[nodiscard]] static FileResult<String> ReadString(const Path& file_path);
};
}  // namespace se
