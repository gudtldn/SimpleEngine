#pragma once
#include <thread>
#include <filesystem>

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


/**
 * 플랫폼 종속적인 기능들에 대한 함수 유틸리티
 */
namespace se::platform
{
/**
 * 특정 스레드의 이름을 설정합니다.
 * @param thread 이름을 설정할 스레드 객체입니다.
 * @param name 설정할 스레드의 이름입니다.
 * @note macOS에서는 다른 스레드의 이름을 설정할 수 없으므로 이 함수가 동작하지 않습니다.
 */
SE_CORE_API void SetThreadName(std::thread& thread, const String& name);

/**
 * 현재 실행 중인 스레드의 이름을 설정합니다.
 * @param name 설정할 스레드의 이름입니다.
 */
SE_CORE_API void SetCurrentThreadName(const String& name);

/**
 * 특정 스레드의 이름을 가져옵니다.
 * @param thread 이름을 가저올 스레드
 * @return 스레드 이름
 */
[[nodiscard]] SE_CORE_API String GetThreadName(std::thread& thread);

/**
 * 이 함수가 호출되었던 스레드의 이름을 가져옵니다.
 * @return 스레드 이름
 */
[[nodiscard]] SE_CORE_API String GetCurrentThreadName();

/**
 * 현재 실행 파일(.exe)이 위치한 디렉터리의 절대 경로를 가져옵니다.
 * @return 실행 파일이 있는 디렉터리의 경로, 실패 시 nullopt
 */
[[nodiscard]] SE_CORE_API std::filesystem::path GetExecutableDirectory();
}
