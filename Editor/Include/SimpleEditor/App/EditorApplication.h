#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEngine/App/Application.h"
#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Types/HashDigest.h"

#include "SDL3/SDL.h"

// forward declaration
namespace se::graphics { struct SceneDrawData; }


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
    /** 현재 프레임에 필요한 메시를 GPU 메모리에 업로드합니다. */
    void EnsureMeshesResident(SDL_GPUCommandBuffer* cmd, const graphics::SceneDrawData& in_scene_data);

    // GPU에 업로드된 메시의 해시 쌍을 추적합니다 (Hot-reload 감지용)
    struct MeshCookKey
    {
        ContentHash source_hash;
        ContentHash settings_hash;

        bool operator==(const MeshCookKey&) const = default;
    };
    HashMap<asset::AssetId, MeshCookKey> uploaded_mesh_hashes;

    SDL_Window* cached_window = nullptr;
};
} // namespace se::editor
