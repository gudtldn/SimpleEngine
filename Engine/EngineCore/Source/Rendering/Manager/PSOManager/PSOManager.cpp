module SimpleEngine.Rendering;
import :Manager.PSOManager;


namespace se::rendering::manager
{
PSOManager::PSOManager(SDL_GPUDevice* in_device)
    : device(in_device)
    , shader_cache(in_device)
{
}

PSOManager::~PSOManager()
{
}

SDL_GPUGraphicsPipeline* PSOManager::GetOrCreateGraphicsPipeline(const SDL_GPUGraphicsPipelineCreateInfo& create_info)
{
    return SDL_CreateGPUGraphicsPipeline(device, &create_info);
}

SDL_GPUComputePipeline* PSOManager::GetOrCreateComputePipeline(const SDL_GPUComputePipelineCreateInfo& create_info)
{
    return SDL_CreateGPUComputePipeline(device, &create_info);
}

void PSOManager::EndFrame()
{
    shader_cache.ClearCache();
}
}
