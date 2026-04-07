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
// SIMD Feature Macros (set by CMake via SE_SIMD_LEVEL option)
// See: Tools/CMake/SESimdOptions.cmake
// Cumulative: AVX2 implies AVX, SSE4_1, SSE2. AVX2 also implies FMA.
// -----------------------------------------------------------------------------
#ifdef SE_CMAKE_SIMD_SSE2
    #define SE_SIMD_SSE2 true
#else
    #define SE_SIMD_SSE2 false
#endif

#ifdef SE_CMAKE_SIMD_SSE4_1
    #define SE_SIMD_SSE4_1 true
#else
    #define SE_SIMD_SSE4_1 false
#endif

#ifdef SE_CMAKE_SIMD_AVX
    #define SE_SIMD_AVX true
#else
    #define SE_SIMD_AVX false
#endif

#ifdef SE_CMAKE_SIMD_AVX2
    #define SE_SIMD_AVX2 true
#else
    #define SE_SIMD_AVX2 false
#endif

#ifdef SE_CMAKE_SIMD_FMA
    #define SE_SIMD_FMA true
#else
    #define SE_SIMD_FMA false
#endif

#ifdef SE_CMAKE_SIMD_NEON
    #define SE_SIMD_NEON true
#else
    #define SE_SIMD_NEON false
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

    // 추후 MSVC 대규모 업데이트시 (ABI 변경시) 매크로 제거
    #define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif SE_COMPILER_CLANG || SE_COMPILER_GCC
    #define FORCE_INLINE __attribute__((always_inline)) __inline__
    #define NO_INLINE __attribute__((noinline))
    #define RESTRICT __restrict__
    #define NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
    #define FORCE_INLINE inline
    #define NO_INLINE
    #define RESTRICT
    #define NO_UNIQUE_ADDRESS [[no_unique_address]]
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

#if SE_BUILD_DEBUG || SE_BUILD_DEVELOPMENT
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

// 크기 및 플랫폼 정수형
using size_t = std::size_t;
using isize = std::ptrdiff_t;
using usize = std::uintptr_t;

/**
 * TMP에서 void를 대체하거나, [[no_unique_address]]와 함께 사용하여
 * Zero-Sized Type 최적화를 수행할 때 사용하는 구조체
 */
struct EmptyType{};
