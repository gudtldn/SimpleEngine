#include "App/EditorApplication.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include "SDL3/SDL.h"


static EditorApplication app;

int main(int argc, char* argv[])
{
    SDL_SetAppMetadata("SimpleEngine_Editor", "0.1.0", "com.editor.simpleengine");

    se::u8string cmd_line;
    for (int i = 1; i < argc; ++i)
    {
        cmd_line += se::utility::string::ToU8String(argv[i]);
        if (i != argc - 1)
        {
            cmd_line += u8" ";
        }
    }

    app.Startup(cmd_line);
    app.Shutdown();

    // SDL_Init(SDL_INIT_VIDEO);
    // SDL_Window* Wnd;
    // SDL_Renderer* Renderer;
    // SDL_CreateWindowAndRenderer("My Window", 1280, 720, SDL_WINDOW_RESIZABLE, &Wnd, &Renderer);
    //
    // bool bRunning = true;
    // while (bRunning)
    // {
    //     SDL_Event Event;
    //     while (SDL_PollEvent(&Event))
    //     {
    //         switch (Event.type)
    //         {
    //         case SDL_EVENT_QUIT:
    //             bRunning = false;
    //             break;
    //         }
    //     }
    //
    //     SDL_RenderClear(Renderer);
    //     SDL_RenderPresent(Renderer);
    // }
    //
    // SDL_DestroyRenderer(Renderer);
    // SDL_DestroyWindow(Wnd);
    // SDL_Quit();

    return 0;
}


