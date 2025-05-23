export module SimpleEngine.Engine;


export class Engine
{
public:
    Engine();
    virtual ~Engine() {}

public:
    virtual void PreInit() {}
    virtual void Init() {}
    virtual void PostInit() {}
};
