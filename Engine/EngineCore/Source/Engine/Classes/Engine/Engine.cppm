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

    virtual void Tick() {}
    virtual void Render() {}
    virtual void PostRender() {}

    virtual void PreShutdown() {}
    virtual void Shutdown() {}
    virtual void PostShutdown() {}
};
