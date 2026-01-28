#pragma once
#include <cstdio>
#include <format>
#include <print>
#include <source_location>
#include <utility>

#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se::details
{
[[nodiscard]] constexpr std::string_view GetPrettyFileName(const char* file_name)
{
    const std::string_view view(file_name);
    const usize last_slash = view.find_last_of("/\\");
    if (last_slash == std::string_view::npos)
    {
        return view;
    }
    return view.substr(last_slash + 1);
}

template <typename... Args>
void PrintLogWithLocation(const std::source_location& loc, std::format_string<Args...> fmt, Args&&... args) noexcept
{
    const std::string user_message = std::format(fmt, std::forward<Args>(args)...);
    const std::string final_log = std::format(
        "[{}:{}] {}",
        GetPrettyFileName(loc.file_name()),
        loc.line(), user_message
    );

    std::println(stderr, "{}", final_log);
    std::fflush(stderr);
}

inline void ReportAssertionFailure(const std::source_location& loc, std::string_view expr) noexcept // NOLINT(*-exception-escape)
{
    std::println(stderr, "[{}:{}] Assertion failed: {}", GetPrettyFileName(loc.file_name()), loc.line(), expr);
    std::fflush(stderr);
}

template <typename... Args>
void ReportAssertionFailure(const std::source_location& loc, std::string_view expr, std::format_string<Args...> fmt, Args&&... args) noexcept
{
    const std::string user_msg = std::format(fmt, std::forward<Args>(args)...);
    std::println(stderr, "[{}:{}] Assertion failed: {}\n└─ {}", GetPrettyFileName(loc.file_name()), loc.line(), expr, user_msg);
    std::fflush(stderr);
}
} // namespace se::details

#if SE_DEBUG_BUILD
    #define SE_ENABLE_DEBUG_TOOLS true
    #define SE_ENABLE_ASSERTS true
#else
    #define SE_ENABLE_DEBUG_TOOLS false
    #define SE_ENABLE_ASSERTS false
#endif

// --- 중단점 (Breakpoint) ---
#if SE_ENABLE_DEBUG_TOOLS
    #if defined(SE_COMPILER_MSVC)
        #define SE_BREAKPOINT() __debugbreak()
    #elif defined(SE_COMPILER_CLANG) || defined(SE_COMPILER_GCC)
        #define SE_BREAKPOINT() __builtin_trap()
    #else
        #include <csignal>
        #if defined(SIGTRAP)
            #define SE_BREAKPOINT() raise(SIGTRAP)
        #else
            #define SE_BREAKPOINT() ((void)0)
        #endif
    #endif
    #define SE_BREAKPOINT_CONDITION(cond) do { if (cond) { SE_BREAKPOINT(); } } while(0)
#else
    #define SE_BREAKPOINT() ((void)0)
    #define SE_BREAKPOINT_CONDITION(cond) ((void)0)
#endif

// --- 치명적 오류 (Fatal Error) ---
// 복구 불가능한 오류가 발생했을 때 사용합니다.
// 오류를 기록하고 디버거를 중단시킨 후 프로그램을 즉시 종료합니다.
#define SE_FATAL_ERROR(message, ...) \
    do \
    { \
        ::se::details::PrintLogWithLocation(std::source_location::current(), "Fatal Error: " message __VA_OPT__(, __VA_ARGS__)); \
        SE_BREAKPOINT(); \
        std::terminate(); \
    } while (0)

// --- 어설션 (Assertions) ---
// 디버그 빌드에서만 조건을 검사하고, 실패 시 프로그램을 중단합니다.
#if SE_ENABLE_ASSERTS
    // 기본 어설션: SE_ASSERT(expression, "optional message", ...args)
    #define SE_ASSERT(expr, ...) \
        do \
        { \
            if (!(!!(expr))) \
            { \
                ::se::details::ReportAssertionFailure(std::source_location::current(), #expr __VA_OPT__(, __VA_ARGS__)); \
                SE_BREAKPOINT(); \
                std::abort(); \
            } \
        } while (0)

    #define SE_ENSURE(expr, ...) \
        (!!(expr) || [&] \
        { \
            ::se::details::ReportAssertionFailure(std::source_location::current(), "Ensure failed (" #expr ")" __VA_OPT__(, __VA_ARGS__)); \
            SE_BREAKPOINT(); \
            return false; \
        }())

#else
    #define SE_ASSERT(expr, ...) ((void)0)
    #define SE_ENSURE(expr, ...) (!!(expr))
#endif

// --- 미구현 / 도달 불가 코드 ---
#if SE_ENABLE_DEBUG_TOOLS
    // 아직 구현하지 않은 로직에 사용합니다.
    #define SE_UNIMPLEMENTED() SE_FATAL_ERROR("Code path is not implemented yet! (file: {}, line: {})", __FILE__, __LINE__)

    // 논리적으로 도달해서는 안 되는 코드 경로에 사용합니다.
    #define SE_UNREACHABLE() SE_FATAL_ERROR("Unreachable code path executed! (file: {}, line: {})", __FILE__, __LINE__)

#else
    #define SE_UNIMPLEMENTED() ((void)0)

    #include <utility>
    #define SE_UNREACHABLE() std::unreachable()
#endif

// --- TO DO 메시지 ---
// 컴파일 시점에 TO DO 메시지를 출력합니다.
#if defined(SE_COMPILER_MSVC)
    #define SE_PRAGMA(x) __pragma(x)
    #define SE_TODO(msg) SE_PRAGMA(message("[" __FILE__ ":" _CRT_STRINGIZE(__LINE__) "]: TODO - " msg))
#elif defined(SE_COMPILER_CLANG) || defined(SE_COMPILER_GCC)
    #define SE_PRAGMA(x) _Pragma(#x)
    #define SE_TODO(msg) SE_PRAGMA(message("TODO: " msg))
#else
    #define SE_TODO(msg)
#endif
