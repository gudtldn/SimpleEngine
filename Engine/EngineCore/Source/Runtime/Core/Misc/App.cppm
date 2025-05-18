module;
#include "EngineCore.h"
export module SimpleEngine.Core:Application;


export class ENGINE_API FApp
{
public:
    static double CurrentTime;
    static double LastTime;
    static double DeltaTime;
    static double FixedDeltaTime;
};
