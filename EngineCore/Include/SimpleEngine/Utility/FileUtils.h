#pragma once
#include <expected>
#include <filesystem>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::utility::file
{
struct FileReadError
{
    enum class Type
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

    Type type;
    String message;

    static FileReadError NotFound(String&& msg) { return { Type::FileNotFound, std::move(msg) }; }
    static FileReadError OpenFailed(String&& msg) { return { Type::FileOpenFailed, std::move(msg) }; }
    static FileReadError Permission(String&& msg) { return { Type::PermissionDenied, std::move(msg) }; }
    static FileReadError Format(String&& msg) { return { Type::InvalidFormat, std::move(msg) }; }
    static FileReadError Read(String&& msg) { return { Type::ReadFailed, std::move(msg) }; }
    static FileReadError Write(String&& msg) { return { Type::WriteFailed, std::move(msg) }; }
    static FileReadError EndOfFile(String&& msg) { return { Type::UnexpectedEOF, std::move(msg) }; }
    static FileReadError OutOfMem(String&& msg) { return { Type::OutOfMemory, std::move(msg) }; }
    static FileReadError Unknown(String&& msg) { return { Type::UnknownError, std::move(msg) }; }
};

template <typename T>
using FileResult = Expected<T, FileReadError>;

/** 파일을 읽고, byte array로 반환합니다. */
[[nodiscard]] SE_CORE_API FileResult<Array<uint8>> ReadToByteArray(const std::filesystem::path& file_path);

/** 파일을 읽고, string으로 반환합니다. */
[[nodiscard]] SE_CORE_API FileResult<String> ReadToString(const std::filesystem::path& file_path);
}
