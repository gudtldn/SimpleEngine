export module SimpleEngine.Rendering:Manager.PSOManager;

import SimpleEngine.Types;
import std;

import "SDL3/SDL_gpu.h";


namespace se::rendering::manager
{
/**
 * Graphics API에 사용되는 PSO를 관리하는 매니저
 */
export class PSOManager
{
public:
    explicit PSOManager(SDL_GPUDevice* in_device);
    ~PSOManager() = default;

private:
    SDL_GPUDevice* device;
};
}
