// ReSharper disable CppUnusedIncludeDirective
#pragma once

#include <cstddef>
#include <cstdint>

// CMake DLL import/export Macro
#include "SimpleEngine/EngineCoreAPI.h"


// -----------------------------------------------------------------------------
// Platform Detection
// -----------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
    #define SE_PLATFORM_WINDOWS true
#else
    #define SE_PLATFORM_WINDOWS false
#endif

#if defined(__linux__)
    #define SE_PLATFORM_LINUX true
#else
    #define SE_PLATFORM_LINUX false
#endif

#if defined(__APPLE__)
    #define SE_PLATFORM_MACOS true
#else
    #define SE_PLATFORM_MACOS false
#endif

// 플랫폼 아키텍처 매크로
// -----------------------------------------------------------------------------
// Architecture Detection
// -----------------------------------------------------------------------------
#if defined(_M_X64) || defined(__x86_64__)
    #define SE_ARCH_X64 true
    #define SE_ARCH_X86 false
    #define SE_ARCH_ARM64 false
#elif defined(_M_IX86) || defined(__i386__)
    #define SE_ARCH_X64 false
    #define SE_ARCH_X86 true
    #define SE_ARCH_ARM64 false
#elif defined(_M_ARM64) || defined(__aarch64__)
    #define SE_ARCH_X64 false
    #define SE_ARCH_X86 false
    #define SE_ARCH_ARM64 true
#else
    #define SE_ARCH_X64 false
    #define SE_ARCH_X86 false
    #define SE_ARCH_ARM64 false
#endif

#if SE_ARCH_X86 || SE_ARCH_X64
    #define SE_ARCH_X86_FAMILY true
#else
    #define SE_ARCH_X86_FAMILY false
#endif

#if SE_ARCH_ARM64
    #define SE_ARCH_ARM_FAMILY true
#else
    #define SE_ARCH_ARM_FAMILY false
#endif

// -----------------------------------------------------------------------------
// Compiler Detection
// -----------------------------------------------------------------------------
#if defined(__clang__)
    #define SE_COMPILER_CLANG true
    #define SE_COMPILER_MSVC false
    #define SE_COMPILER_GCC false
#elif defined(_MSC_VER)
    #define SE_COMPILER_CLANG false
    #define SE_COMPILER_MSVC true
    #define SE_COMPILER_GCC false
#elif defined(__GNUC__)
    #define SE_COMPILER_CLANG false
    #define SE_COMPILER_MSVC false
    #define SE_COMPILER_GCC true
#else
    #define SE_COMPILER_CLANG false
    #define SE_COMPILER_MSVC false
    #define SE_COMPILER_GCC false
#endif

// -----------------------------------------------------------------------------
// Utility Macros
// -----------------------------------------------------------------------------
#if SE_COMPILER_MSVC
    #define FORCE_INLINE __forceinline
    #define NO_INLINE __declspec(noinline)
    #define RESTRICT __restrict
#elif SE_COMPILER_CLANG || SE_COMPILER_GCC
    #define FORCE_INLINE __attribute__((always_inline)) __inline__
    #define NO_INLINE __attribute__((noinline))
    #define RESTRICT __restrict__
#else
    #define FORCE_INLINE inline
    #define NO_INLINE
    #define RESTRICT
#endif

// -----------------------------------------------------------------------------
// Build Configuration
// -----------------------------------------------------------------------------
#ifdef SE_CMAKE_CONFIGURATION_DEBUG
    #define SE_BUILD_DEBUG true
    #define SE_DEBUG_EXPRESSION(x) x
#else
    #define SE_BUILD_DEBUG false
    #define SE_DEBUG_EXPRESSION(x)
#endif

#ifdef SE_CMAKE_CONFIGURATION_DEVELOPMENT
    #define SE_BUILD_DEVELOPMENT true
    #define SE_DEVELOPMENT_EXPRESSION(x) x
#else
    #define SE_BUILD_DEVELOPMENT false
    #define SE_DEVELOPMENT_EXPRESSION(x)
#endif

#ifdef SE_CMAKE_CONFIGURATION_RELEASE
    #define SE_BUILD_RELEASE true
    #define SE_RELEASE_EXPRESSION(x) x
#else
    #define SE_BUILD_RELEASE false
    #define SE_RELEASE_EXPRESSION(x)
#endif

#ifdef SE_CMAKE_OPTION_ENABLE_ASSERTS
    #define SE_ENABLE_ASSERTS true
#else
    #define SE_ENABLE_ASSERTS false
#endif

#if SE_BUILD_DEBUG || SE_BUILD_DEVELOPMENT || SE_ENABLE_ASSERTS
    #define SE_ENABLE_DEBUG_TOOLS true
#else
    #define SE_ENABLE_DEBUG_TOOLS false
#endif


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
using isize = std::ptrdiff_t;
using usize = std::uintptr_t;
