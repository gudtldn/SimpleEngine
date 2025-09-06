import std;
import SE.Core;
import SE.Editor.App;

import <Windows.h>;
import <SDL3/SDL.h>;

namespace
{
se::core::memory::memory_resource::TrackedMemoryResource DefaultTrackedResource;

class PmrScopeGuard
{
public:
    PmrScopeGuard()
        : original_resource(std::pmr::set_default_resource(&DefaultTrackedResource))
    {
    }

    ~PmrScopeGuard()
    {
        std::pmr::set_default_resource(original_resource);
    }

private:
    std::pmr::memory_resource* original_resource;
};
}

static EditorApplication app;

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nShowCmd);

    SDL_SetAppMetadata("SimpleEngine_Editor", "0.1.0", "com.editor.simpleengine");

    {
        PmrScopeGuard scope_guard;

        app.Startup(lpCmdLine);
        app.Shutdown();
    }

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


