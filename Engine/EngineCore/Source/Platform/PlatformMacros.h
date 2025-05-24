// ReSharper disable CppClangTidyClangDiagnosticUnusedMacros
#pragma once


// 플랫폼 매크로
#if defined(_WIN32) || defined(_WIN64) // Windows
    #define PLATFORM_WINDOWS 1
#elif defined(__linux__) // Linux
    #define PLATFORM_LINUX 1
#elif defined(__APPLE__) // MacOS
    #define PLATFORM_MACOS 1
#else
    #error "Unsupported platform"
#endif


#if !defined(PLATFORM_WINDOWS)
    #define PLATFORM_WINDOWS 0
#endif

#if !defined(PLATFORM_LINUX)
    #define PLATFORM_LINUX 0
#endif

#if !defined(PLATFORM_MACOS)
    #define PLATFORM_MACOS 0
#endif


#if PLATFORM_WINDOWS
    #include "Platform/Windows/WindowsPlatform.h"
#elif PLATFORM_LINUX
    #include "Platform/Linux/LinuxPlatform.h"
#elif PLATFORM_MACOS
    #include "Platform/MacOS/MacOSPlatform.h"
#endif


// 컴파일러별 매크로
#ifdef _MSC_VER // MSVC 컴파일러
    #define FORCE_INLINE __forceinline
#elif ((defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)) // GNU && Clang 컴파일러
    #define FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
    #define FORCE_INLINE inline
#endif
