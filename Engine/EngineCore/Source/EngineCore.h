#pragma once

#ifdef ENGINECORE_EXPORTS
    #define ENGINE_API __declspec(dllexport)
#else // CoreEngine.dll을 사용하는 프로젝트에서
    #define ENGINE_API __declspec(dllimport)
#endif
