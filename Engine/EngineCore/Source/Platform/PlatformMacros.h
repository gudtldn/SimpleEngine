// ReSharper disable CppClangTidyClangDiagnosticUnusedMacros
#pragma once

// Windows
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
// Linux
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
// MacOS
#elif defined(__APPLE__)
    #define PLATFORM_MACOS 1
#else
    #define PLATFORM_UNKNOWN 1
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
