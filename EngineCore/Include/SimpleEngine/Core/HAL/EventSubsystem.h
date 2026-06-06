#pragma once

#include "SimpleEngine/Core/Functional/MultiDelegate.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "SDL3/SDL.h"


namespace se
{
/**
 * SDL 이벤트 폴링 및 Raw 이벤트 브로드캐스팅을 담당하는 Subsystem
 */
class SE_CORE_API SE_ANNOTATION(=meta::Reflect, =meta::Hidden, =meta::Transient) EventSubsystem : public SubsystemBase
{
    SE_CLASS(EventSubsystem, SubsystemBase)

public:
    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

public:
    /** 매 프레임 호출하여 SDL 이벤트를 폴링하고 Delegate를 Broadcast합니다. */
    void PollEvents();

public:
    /**
     * SDL 이벤트가 발생할 때마다 Broadcast됩니다.
     * ImGui 등 모든 raw SDL 이벤트를 받아야 하는 시스템용입니다.
     */
    MultiDelegate<void(const SDL_Event&)> on_sdl_event;

    /**
     * SDL_EVENT_QUIT 이벤트 발생 시 Broadcast됩니다.
     */
    MultiDelegate<void()> on_quit_requested;

    /**
     * OS에서 파일이 드롭되었을 때 Broadcast됩니다. (탐색기 -> 에디터 창)
     * 드롭된 파일의 물리 경로가 전달됩니다.
     */
    MultiDelegate<void(const Path& file_path)> on_file_dropped;
};
} // namespace se
