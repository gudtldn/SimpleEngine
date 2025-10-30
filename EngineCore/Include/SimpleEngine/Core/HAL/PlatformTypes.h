// ReSharper disable CppUnusedIncludeDirective
#pragma once

#include <cstddef>
#include <cstdint>

// CMake DLL import/export Macro
#include <enginecore_export.h>


// 플랫폼 매크로
#if defined(_WIN32) || defined(_WIN64) // Windows
#define SE_PLATFORM_WINDOWS true
#elif defined(__linux__) // Linux
#define SE_PLATFORM_LINUX true
#elif defined(__APPLE__) // MacOS
#define SE_PLATFORM_MACOS true
#else
#error "Unsupported platform"
#endif

#if !defined(SE_PLATFORM_WINDOWS)
#define SE_PLATFORM_WINDOWS false
#endif

#if !defined(SE_PLATFORM_LINUX)
#define SE_PLATFORM_LINUX false
#endif

#if !defined(SE_PLATFORM_MACOS)
#define SE_PLATFORM_MACOS false
#endif
// ~플랫폼 매크로

// 플랫폼 아키텍처 매크로
#if defined(_M_X64) || defined(__x86_64__)
#define SE_COMPILE_PLATFORM_X86_64 true
#elif defined(_M_IX86) || defined(__i386__)
#define SE_COMPILE_PLATFORM_X86 true
#elif defined(_M_ARM64) || defined(__aarch64__)
#define SE_COMPILE_PLATFORM_ARM64 true
#else
#error "Unsupported platform"
#endif

#if defined(SE_COMPILE_PLATFORM_X86) || defined(SE_COMPILE_PLATFORM_X86_64)
#define SE_COMPILE_PLATFORM_X86_FAMILY true
#else
#define SE_COMPILE_PLATFORM_X86_FAMILY false
#endif

#if defined(SE_COMPILE_PLATFORM_ARM64)
#define SE_COMPILE_PLATFORM_ARM_FAMILY true
#else
#define SE_COMPILE_PLATFORM_ARM_FAMILY false
#endif

#if !defined(SE_COMPILE_PLATFORM_X86_64)
#define SE_COMPILE_PLATFORM_X86_64 false
#endif
#if !defined(SE_COMPILE_PLATFORM_X86)
#define SE_COMPILE_PLATFORM_X86 false
#endif
#if !defined(SE_COMPILE_PLATFORM_ARM64)
#define SE_COMPILE_PLATFORM_ARM64 false
#endif
// ~플랫폼 아키텍처 매크로

// 컴파일러별 매크로
#ifdef _MSC_VER // MSVC 컴파일러
#define FORCE_INLINE __forceinline
#define NO_INLINE __declspec(noinline)
#elif ((defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)) // GNU && Clang 컴파일러
#define FORCE_INLINE __attribute__((always_inline)) __inline__
#define NO_INLINE __attribute__((noinline))
#else
#define FORCE_INLINE inline
#define NO_INLINE
#endif
// ~컴파일러별 매크로

// 디버그/릴리즈 빌드 매크로
#if defined(_DEBUG)
#define SE_DEBUG_BUILD true
#define SE_DEBUG_EXPRESION(x) x
#else
#define SE_DEBUG_BUILD false
#define SE_DEBUG_EXPRESION(x)
#endif

#if defined(NDEBUG)
#define SE_RELEASE_BUILD true
#define SE_RELEASE_EXPRESION(x) x
#else
#define SE_RELEASE_BUILD false
#define SE_RELEASE_EXPRESION(x)
#endif
// ~디버그/릴리즈 빌드 매크로


// 정수형
using int8 = std::int8_t;
using uint8 = std::uint8_t;
using int16 = std::int16_t;
using uint16 = std::uint16_t;
using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

// 문자형 (UTF-8 기본)
using char8 = char8_t;
using char16 = char16_t;
using char32 = char32_t;

// 크기 및 플랫폼 정수형
using size_t = std::size_t;
using isize = std::intptr_t;
using usize = std::uintptr_t;
