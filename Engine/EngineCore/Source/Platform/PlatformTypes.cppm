export module SimpleEngine.Platform.Types;
import <cstddef>;
import <cstdint>;

export
{
    // 정수형
    using int8    = std::int8_t;
    using uint8   = std::uint8_t;
    using int16   = std::int16_t;
    using uint16  = std::uint16_t;
    using int32   = std::int32_t;
    using uint32  = std::uint32_t;
    using int64   = std::int64_t;
    using uint64  = std::uint64_t;

    // 문자형 (UTF-8 기본)
    using char8   = char8_t;
    using char16  = char16_t;
    using char32  = char32_t;

    // 크기 및 포인터 정수형
    using size_t  = std::size_t;
    using intptr  = std::intptr_t;
    using uintptr = std::uintptr_t;
}
