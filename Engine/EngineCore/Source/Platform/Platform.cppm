export module SE.Platform;
export import :Detection;

import SE.Types;
import std;


export namespace se::platform
{
/**
 * 플랫폼 종속적인 기능들에 대한 함수 유틸리티
 */
struct Platform
{
    /**
     * 특정 스레드의 이름을 설정합니다.
     * @param thread 이름을 설정할 스레드 객체입니다.
     * @param name 설정할 스레드의 이름입니다.
     * @note macOS에서는 다른 스레드의 이름을 설정할 수 없으므로 이 함수가 동작하지 않습니다.
     */
    static void SetThreadName(std::thread& thread, const std::u8string& name);

    /**
     * @brief 현재 실행 중인 스레드의 이름을 설정합니다.
     * @param name 설정할 스레드의 이름입니다.
     */
    static void SetCurrentThreadName(const std::u8string& name);
};
}
