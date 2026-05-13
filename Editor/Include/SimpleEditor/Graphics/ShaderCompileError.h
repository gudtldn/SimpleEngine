#pragma once

#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
/**
 * 셰이더 컴파일 과정에서 발생하는 에러
 */
class ShaderCompileError final : public IError
{
public:
    enum class EType : u8
    {
        FileNotFound,  // 소스 파일을 찾을 수 없음
        ReadFailed,    // 소스 파일 읽기 실패
        StageMismatch, // 셰이더 스테이지 판별 실패
        CompileFailed, // DXC 컴파일 실패 (HLSL -> SPIR-V)
        NotSupported,  // 현재 플랫폼에서 컴파일 미지원
        NoPragma,      // #pragma se_shader가 없음
    };
    using enum EType;

    ShaderCompileError(EType type, String message, Path source_path = {})
        : type(type)
        , message(std::move(message))
        , source_path(std::move(source_path))
    {
    }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] EType GetType() const noexcept { return type; }
    [[nodiscard]] const Path& GetSourcePath() const noexcept { return source_path; }

private:
    EType type;
    String message;
    Path source_path;
};

template <typename T>
using ShaderCompileResult = Expected<T, ShaderCompileError>;
} // namespace se::editor
