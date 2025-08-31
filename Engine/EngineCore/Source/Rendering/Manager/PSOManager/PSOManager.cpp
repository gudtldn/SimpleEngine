module SimpleEngine.Rendering;
import :Manager.PSOManager;


namespace se::rendering::manager
{
PSOManager::PSOManager(SDL_GPUDevice* in_device)
    : device(in_device)
    , shader_cache(in_device)
{
}

SDL_GPUGraphicsPipeline* PSOManager::CreateGraphicsPipeline(const SDL_GPUGraphicsPipelineCreateInfo& create_info)
{
    return SDL_CreateGPUGraphicsPipeline(device, &create_info);
}

SDL_GPUComputePipeline* PSOManager::CreateComputePipeline(const SDL_GPUComputePipelineCreateInfo& create_info)
{
    return SDL_CreateGPUComputePipeline(device, &create_info);
}

void PSOManager::EndFrame()
{
    shader_cache.ClearCache();
}
}
