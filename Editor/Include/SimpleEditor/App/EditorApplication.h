#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEngine/App/Application.h"

#include "SDL3/SDL_gpu.h"

// forward declaration
namespace se { struct FramePacket; }


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
    // virtual void Update(f32 delta_time) override;

    virtual void Render() override;

private:
    /** Asset Load 및 residency 체크 후 FramePacket의 mesh/texture_upload_requests를 채웁니다. */
    void PrepareGpuUploads(FramePacket& fp);

    /** GPU Upload를 수행합니다. */
    void ExecuteGpuUploads(SDL_GPUCommandBuffer* cmd, const FramePacket& fp);
};
} // namespace se::editor
