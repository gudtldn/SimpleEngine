export module SimpleEngine.Subsystems.PlatformSubsystem;

import SimpleEngine.Core.ISubsystem;
import std;
import <SDL3/SDL.h>;


export class PlatformSubsystem : public ISubsystem
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

public:
    SDL_Window* GetWindow() const { return window; }

private:
    SDL_Window* window = nullptr;
};
