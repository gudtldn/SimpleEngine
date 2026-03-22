// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Graphics/RenderGraph/RenderGraphBuilder.h"

#include "SimpleEngine/Core/Logging/Logging.h"

#include <utility>


namespace se::graphics
{
RGTextureHandle RenderGraphBuilder::ImportTexture(const StringName& name, SDL_GPUTexture* texture)
{
    const RGTextureHandle handle = RegisterTextureSlot(name);
    RGResourceNode& node = resource_nodes[handle.index];

    if (node.resource)
    {
        ConsoleLog(ELogLevel::Error, "Duplicate resource name in ImportTexture: {}", name.ToString());
        return handle;
    }

    node.resource = std::make_unique<RGExternalTexture>(texture);
    return handle;
}

RGBufferHandle RenderGraphBuilder::ImportBuffer(const StringName& name, SDL_GPUBuffer* buffer)
{
    const RGBufferHandle handle = RegisterBufferSlot(name);
    RGResourceNode& node = resource_nodes[handle.index];

    if (node.resource)
    {
        ConsoleLog(ELogLevel::Error, "Duplicate resource name in ImportBuffer: {}", name.ToString());
        return handle;
    }

    node.resource = std::make_unique<RGExternalBuffer>(buffer);
    return handle;
}

RGTextureHandle RenderGraphBuilder::CreateTexture(const StringName& name, const SDL_GPUTextureCreateInfo& desc)
{
    const RGTextureHandle handle = RegisterTextureSlot(name);
    RGResourceNode& node = resource_nodes[handle.index];

    if (node.resource)
    {
        ConsoleLog(ELogLevel::Error, "Duplicate resource name in CreateTexture: {}", name.ToString());
        return handle;
    }

    auto texture_resource = std::make_unique<RGTransientTexture>();
    texture_resource->description = desc;
    node.resource = std::move(texture_resource);
    return handle;
}

RGBufferHandle RenderGraphBuilder::CreateBuffer(const StringName& name, const SDL_GPUBufferCreateInfo& desc)
{
    const RGBufferHandle handle = RegisterBufferSlot(name);
    RGResourceNode& node = resource_nodes[handle.index];

    if (node.resource)
    {
        ConsoleLog(ELogLevel::Error, "Duplicate resource name in CreateBuffer: {}", name.ToString());
        return handle;
    }

    auto buffer_resource = std::make_unique<RGTransientBuffer>();
    buffer_resource->description = desc;
    node.resource = std::move(buffer_resource);
    return handle;
}

void RenderGraphBuilder::AddPassInternal(StringName name, std::unique_ptr<RenderPassBase> pass)
{
    RGPassNode& node = pass_nodes.Emplace();
    node.name = name;
    node.pass_object = std::move(pass);
}

void RenderGraphBuilder::Clear()
{
    pass_nodes.Clear();
    resource_nodes.Clear();
    resource_name_map.Clear();
}

RGTextureHandle RenderGraphBuilder::RegisterTextureSlot(const StringName& name)
{
    if (const Optional existing = resource_name_map.Find(name))
    {
        return RGTextureHandle{ .index = *existing };
    }

    RGResourceNode node;
    node.name = name;
    const uint32 index = static_cast<uint32>(resource_nodes.Len());
    resource_nodes.Push(std::move(node));
    resource_name_map[name] = index;
    return RGTextureHandle{ .index = index };
}

RGBufferHandle RenderGraphBuilder::RegisterBufferSlot(const StringName& name)
{
    if (const Optional existing = resource_name_map.Find(name))
    {
        return RGBufferHandle{ .index = *existing };
    }

    RGResourceNode node;
    node.name = name;
    const uint32 index = static_cast<uint32>(resource_nodes.Len());
    resource_nodes.Push(std::move(node));
    resource_name_map[name] = index;
    return RGBufferHandle{ .index = index };
}
} // namespace se::graphics