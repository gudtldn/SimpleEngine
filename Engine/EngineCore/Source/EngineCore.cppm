export module EngineCore;

#ifdef ENGINECORE_EXPORTS
    #define COREENGINE_API __declspec(dllexport)
#else // CoreEngine.dll을 사용하는 프로젝트에서
    #define COREENGINE_API __declspec(dllimport)
#endif
