// ReSharper disable CppClangTidyClangDiagnosticUnusedMacros
#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define DEBUG_EXPRESION(x) x
#else
#define DEBUG_EXPRESION(x)
#endif

#if defined(NDEBUG)
#define RELEASE_EXPRESION(x) x
#else
#define RELEASE_EXPRESION(x)
#endif
