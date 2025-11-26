#pragma once
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Functional/Function.h"

#include "SDL3/SDL.h"


namespace se::core
{
struct FileFilter
{
    const char* name;    // 예: "Image Files"
    const char* pattern; // 예: "png;jpg;jpeg"
};

/**
 * 플랫폼 네이티브 파일 다이얼로그를 띄우는 유틸리티 클래스
 */
class SE_CORE_API FileDialog
{
public:
    // 파일 선택 시 호출될 콜백 (경로가 비어있으면 취소/에러)
    using OnFileSelected = Function<void(const String& path)>;
    using OnMultiFilesSelected = Function<void(const Array<String>& paths)>;

public:
    /**
     * 파일 열기 다이얼로그 (단일 선택)
     * @param callback 사용자가 파일을 선택하면 호출될 `void(const String& path)` 형식의 함수
     * @param filters 필터 목록 (예: { {"Images", "png;jpg"}, {"All", "*"} })
     * @param default_location 초기 경로 (nullptr 가능)
     * @param window 부모 윈도우 (nullptr이면 현재 포커스된 윈도우 사용)
     */
    static void OpenFile(
        OnFileSelected callback,
        const Array<FileFilter>& filters = {},
        const char* default_location = nullptr,
        SDL_Window* window = nullptr
    );

    /**
     * 파일 열기 다이얼로그 (다중 선택)
     * @param callback 사용자가 파일을 선택하면 호출될 `void(const Array<String>& paths)` 형식의 함수
     * @param filters 필터 목록 (예: { {"Images", "png;jpg"}, {"All", "*"} })
     * @param default_location 초기 경로 (nullptr 가능)
     * @param window 부모 윈도우 (nullptr이면 현재 포커스된 윈도우 사용)
     */
    static void OpenFiles(
        OnMultiFilesSelected callback,
        const Array<FileFilter>& filters = {},
        const char* default_location = nullptr,
        SDL_Window* window = nullptr
    );

    /**
     * 파일 저장 다이얼로그
     * @param callback 사용자가 파일을 선택하면 호출될 `void(const String& path)` 형식의 함수
     * @param filters 필터 목록 (예: { {"Images", "png;jpg"}, {"All", "*"} })
     * @param default_location 초기 경로 (nullptr 가능)
     * @param window 부모 윈도우 (nullptr이면 현재 포커스된 윈도우 사용)
     */
    static void SaveFile(
        OnFileSelected callback,
        const Array<FileFilter>& filters = {},
        const char* default_location = nullptr,
        SDL_Window* window = nullptr
    );
};
}
