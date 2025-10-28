#include "App/EditorApplication.h"
#include "SimpleEngine/Core/Container/String.h"

#include "SDL3/SDL.h"


int main(int argc, char* argv[])
{
    static EditorApplication app;
    SDL_SetAppMetadata("SimpleEngine_Editor", "0.1.0", "com.editor.simpleengine");

    se::String cmd_line;
    for (int i = 1; i < argc; ++i)
    {
        cmd_line += argv[i];
        if (i != argc - 1)
        {
            cmd_line += " ";
        }
    }

    app.Startup(cmd_line);
    app.Shutdown();

    return 0;
}


