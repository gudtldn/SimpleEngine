#include "SimpleEngine/Core/HAL/Platform.h"

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
        constexpr uint32 max_traversal_depth = 10;

        // TODO: 나중에 실제 프로젝트에 맞는 이름을 자동으로 찾도록 수정
        constexpr const char* sentinel_file_name = "SimpleEngine.project";

        Path current = GetExecutableDirectory();

        for (uint32 i = 0; i < max_traversal_depth; ++i)
        {
            if ((current / sentinel_file_name).Exists())
            {
                return current;
            }

            const Optional<Path> parent = current.Parent();
            if (!parent.HasValue() || parent.Value() == current)
            {
                break; // 파일 시스템 루트에 도달
            }
            current = parent.Value();
        }

        // 센티넬 파일을 찾지 못한 경우, 실행 파일 디렉토리를 폴백으로 반환
        return GetExecutableDirectory();
    }();

    return cached;
}
}  // namespace se
