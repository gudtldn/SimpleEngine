module;

#include "EngineCore.h"

export module SimpleEngine.Core:Engine;


export class ENGINE_API Engine
{
public:
    Engine();
    virtual ~Engine() {}

public:
    virtual void PreInit() {}
    virtual void Init() {}
    virtual void PostInit() {}
};
