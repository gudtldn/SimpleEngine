module;
#include "EngineCore.h"
export module SimpleEngine.Core:Engine;


export class ENGINE_API UEngine
{
public:
    UEngine();
    virtual ~UEngine() {}

public:
    virtual void PreInit() {}
    virtual void Init() {}
    virtual void PostInit() {}
};
