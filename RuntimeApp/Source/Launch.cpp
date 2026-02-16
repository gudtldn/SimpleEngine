#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Core/Container/String.h"

#include "SDL3/SDL.h"


/**
 * 게임 런타임 전용 Application
 * Editor 없이 순수 엔진 + 게임 로직만 실행합니다.
 */
class RuntimeApplication : public se::Application
{
public:
    RuntimeApplication()
        : Application(se::EApplicationMode::GameClient)
    {
    }
};


int main(int argc, char* argv[])
{
    // TODO: Runtime Logic 작성

    // static RuntimeApplication app;
    // SDL_SetAppMetadata("SimpleEngine_Runtime", "0.1.0", "com.runtime.simpleengine");
    //
    // se::String cmd_line;
    // for (int i = 1; i < argc; ++i)
    // {
    //     cmd_line += argv[i];
    //     if (i != argc - 1)
    //     {
    //         cmd_line += " ";
    //     }
    // }
    //
    // app.Startup(cmd_line);
    // app.Shutdown();

    return 0;
}
