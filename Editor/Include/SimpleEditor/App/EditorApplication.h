#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEngine/App/Application.h"

#include "SDL3/SDL.h"


namespace se::editor
{
class SE_EDITOR_API EditorApplication : public se::Application
{
public:
    EditorApplication();

    virtual void Startup(const String& cmd_line) override;

protected:
    virtual void RegisterSubsystems() override;
    virtual bool PostInitialize() override;
    // virtual void PreRelease() override;
    // virtual void Update(float delta_time) override;

    virtual void Render() override;

private:
    SDL_Window* cached_window = nullptr;
};
} // namespace se::editor
