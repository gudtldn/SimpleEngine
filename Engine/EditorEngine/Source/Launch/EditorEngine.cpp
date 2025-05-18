import std;
import SimpleEngine.Core;

import <SDL3/SDL.h>;
import <Windows.h>;


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

    Engine engine = Engine();
    engine.PreInit();
    engine.Init();
    engine.PostInit();

    std::println("Hello World!");
    return 0;
}
