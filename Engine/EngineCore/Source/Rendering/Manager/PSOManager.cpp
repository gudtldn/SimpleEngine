module SimpleEngine.Rendering;
import :Manager.PSOManager;


namespace se::rendering::manager
{
PSOManager::PSOManager(SDL_GPUDevice* in_device)
    : device(in_device)
{
}
}
