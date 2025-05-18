module;

#include "EngineCore.h"

export module SimpleEngine.Core;
import std;


export class CORE_API Engine
{
public:
    Engine();
    virtual ~Engine() {}

public:
    virtual void PreInit() {}
    virtual void Init() {}
    virtual void PostInit() {}
};
