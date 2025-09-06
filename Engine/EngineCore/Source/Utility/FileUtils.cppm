export module SE.Utility:FileUtils;

import SE.Types;
import std;


export namespace se::utility::file_utils
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
    u8string message;

    static FileReadError NotFound(u8string&& msg) { return { Type::FileNotFound, std::move(msg) }; }
    static FileReadError OpenFailed(u8string&& msg) { return { Type::FileOpenFailed, std::move(msg) }; }
    static FileReadError Permission(u8string&& msg) { return { Type::PermissionDenied, std::move(msg) }; }
    static FileReadError Format(u8string&& msg) { return { Type::InvalidFormat, std::move(msg) }; }
    static FileReadError Read(u8string&& msg) { return { Type::ReadFailed, std::move(msg) }; }
    static FileReadError Write(u8string&& msg) { return { Type::WriteFailed, std::move(msg) }; }
    static FileReadError EOF(u8string&& msg) { return { Type::UnexpectedEOF, std::move(msg) }; }
    static FileReadError OutOfMem(u8string&& msg) { return { Type::OutOfMemory, std::move(msg) }; }
    static FileReadError Unknown(u8string&& msg) { return { Type::UnknownError, std::move(msg) }; }
};

template <typename T>
using FileResult = std::expected<T, FileReadError>;

/** 파일을 읽고, byte array로 반환합니다. */
[[nodiscard]] FileResult<vector<uint8>> ReadToByteArray(const std::filesystem::path& file_path);

/** 파일을 읽고, string으로 반환합니다. */
[[nodiscard]] FileResult<u8string> ReadToString(const std::filesystem::path& file_path);
}
