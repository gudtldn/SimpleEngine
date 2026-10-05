#pragma once

#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
/** 셰이더 쿡 과정에서 발생하는 에러 */
class ShaderCookError final : public IError
{
public:
    enum class EType : u8
    {
        ReadFailed,    // 소스 파일 읽기 실패
        CompileFailed, // 컴파일 또는 링크 실패
        NoEntryPoint,  // [shader] 속성이 붙은 진입점이 없음
        NotSupported,  // 지원하지 않는 스테이지 또는 플랫폼
    };
    using enum EType;

    ShaderCookError(EType type, String message, Path source_path = {})
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
using ShaderCookResult = Expected<T, ShaderCookError>;
} // namespace se::editor
