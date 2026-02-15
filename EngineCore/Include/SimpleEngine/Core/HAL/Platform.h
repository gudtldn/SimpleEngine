#pragma once
#include <thread>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se
{
/**
 * 플랫폼 종속적인 기능들에 대한 유틸리티 구조체
 */
struct SE_CORE_API Platform
{
    /**
     * 특정 스레드의 이름을 설정합니다.
     * @param thread 이름을 설정할 스레드 객체입니다.
     * @param name 설정할 스레드의 이름입니다.
     * @note macOS에서는 다른 스레드의 이름을 설정할 수 없으므로 이 함수가 동작하지 않습니다.
     */
    static void SetThreadName(std::thread& thread, const String& name);

    /**
     * 현재 실행 중인 스레드의 이름을 설정합니다.
     * @param name 설정할 스레드의 이름입니다.
     */
    static void SetCurrentThreadName(const String& name);

    /**
     * 특정 스레드의 이름을 가져옵니다.
     * @param thread 이름을 가저올 스레드
     * @return 스레드 이름
     */
    [[nodiscard]] static String GetThreadName(std::thread& thread);

    /**
     * 이 함수가 호출되었던 스레드의 이름을 가져옵니다.
     * @return 스레드 이름
     */
    [[nodiscard]] static String GetCurrentThreadName();

    /**
     * 현재 실행 파일(.exe)이 위치한 디렉터리의 절대 경로를 가져옵니다.
     * @return 실행 파일이 있는 디렉터리의 경로
     */
    [[nodiscard]] static Path GetExecutableDirectory();

    /**
     * 프로젝트 루트 디렉토리를 탐색합니다.
     *
     * 실행 파일 위치에서 상위 디렉토리를 순회하며 SimpleEngine.project 센티넬 파일을 찾습니다.
     * 결과는 내부적으로 캐싱되어 이후 호출 시 즉시 반환됩니다.
     *
     * @return 프로젝트 루트 디렉토리 경로. 센티넬 파일을 찾지 못하면 실행 파일 디렉토리를 반환합니다.
     */
    [[nodiscard]] static Path FindProjectRoot();

    /**
     * 운영체제의 파일 탐색기를 열어 해당 경로를 보여줍니다.
     */
    static void RevealInExplorer(const Path& path);
};
}  // namespace se
