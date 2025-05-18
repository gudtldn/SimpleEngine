export module SimpleEngine.Core:Engine;


export class UEngine
{
public:
    UEngine();
    virtual ~UEngine() {}

public:
    virtual void PreInit() {}
    virtual void Init() {}
    virtual void PostInit() {}
};
