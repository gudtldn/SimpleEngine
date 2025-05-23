import std;
import SimpleEngine.App;

import <Windows.h>;
import <SDL3/SDL.h>;


int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    Application::Startup(lpCmdLine);
    Application::Shutdown();

    // SDL_SetAppMetadata("My", "0.1.0", "com.example.simpleengine");
    //
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


