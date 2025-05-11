export module EngineCore;

#ifdef ENGINERUNTIME_EXPORTS
    #define COREENGINE_API __declspec(dllexport)
#else // CoreEngine.dll을 사용하는 프로젝트에서
    #define COREENGINE_API __declspec(dllimport)
#endif
