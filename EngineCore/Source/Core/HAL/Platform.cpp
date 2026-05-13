#include "SimpleEngine/Core/HAL/Platform.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Utility/Debug.h"

#include "SDL3/SDL.h"


namespace se
{
Path Platform::GetExecutableDirectory()
{
    return { SDL_GetBasePath() };
}

Path Platform::FindProjectRoot()
{
    // 최초 호출 시 탐색 후 캐싱
    static const Path cached = [] static -> Path
    {
        constexpr u32 MAX_TRAVERSAL_DEPTH = 10;
        constexpr const char* SENTINEL_EXTENSION = ".seproject";

        Path current = GetExecutableDirectory();

        for (u32 i = 0; i < MAX_TRAVERSAL_DEPTH; ++i)
        {
            // 현재 디렉토리에서 *.seproject 파일 탐색
            for (const DirectoryEntry& entry : FileSystem::ReadDir(current))
            {
                if (entry.IsFile())
                {
                    if (const auto ext = entry.GetPath().Extension())
                    {
                        if (ext == SENTINEL_EXTENSION)
                        {
                            return current;
                        }
                    }
                }
            }

            Optional<Path> parent = current.Parent();
            if (!parent.HasValue() || *parent == current)
            {
                break; // 파일 시스템 루트에 도달
            }
            current = std::move(parent).Value();
        }

        // 센티넬 파일을 찾지 못한 경우, 실행 파일 디렉토리를 폴백으로 반환
        const Path fallback = GetExecutableDirectory();
        SE_ENSURE(false, "Failed to find '*.seproject' sentinel file. Using executable directory as project root: {}", fallback);
        return fallback;
    }();

    return cached;
}
} // namespace se
