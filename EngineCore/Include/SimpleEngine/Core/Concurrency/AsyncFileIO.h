#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Functional/UniqueFunction.h"

#include "SDL3/SDL.h"

#include <memory>
#include <thread>


namespace se
{
// Forward Declaration
template <typename T>
class JobTask;
class Path;

namespace detail
{
struct SDLFreeDeleter
{
    void operator()(void* ptr) const noexcept
    {
        SDL_free(ptr);
    }
};
} // namespace detail

/**
 * 비동기 I/O 결과를 담는 구조체
 *
 * ReadFile / ReadFileAsync의 완료 시 전달되며,
 * success가 false인 경우 data는 비어 있을 수 있습니다.
 */
struct IOResult
{
    /** 읽어들인 파일 데이터 */
    std::unique_ptr<u8[], detail::SDLFreeDeleter> data_ptr = nullptr;

    /** 데이터 byte 사이즈 */
    usize data_len = 0;

public:
    /**
     * 파일 데이터가 정상적으로 읽혔는지 확인합니다.
     * @note 빈 파일(0바이트) 읽기는 실패와 동일하게 취급합니다.
     */
    [[nodiscard]] bool Success() const noexcept
    {
        return data_ptr != nullptr;
    }

    /** 파일 데이터 읽기에 실패했는지 확인합니다. */
    [[nodiscard]] bool Fail() const noexcept
    {
        return data_ptr == nullptr;
    }

    /** 데이터가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept
    {
        return data_len == 0;
    }

    /** 데이터 View를 반환합니다. */
    [[nodiscard]] ArrayView<const u8> AsView() const noexcept
    {
        return { data_ptr.get(), data_len };
    }

    explicit operator bool() const noexcept { return Success(); }
};


/**
 * SDL3 AsyncIO 기반 비동기 파일 I/O (전역 싱글톤)
 *
 * SDL_AsyncIOQueue를 감시하는 Poller Thread가 I/O 완료를 감지하면,
 * JobSystem의 Compute Worker에 콜백/코루틴 재개를 스케줄링합니다.
 * Poller Thread는 절대로 사용자 코드를 직접 실행하지 않습니다.
 */
class SE_CORE_API AsyncFileIO
{
    static AsyncFileIO* instance;

public:
    explicit AsyncFileIO();
    ~AsyncFileIO();

    // 복사 & 이동 금지
    AsyncFileIO(const AsyncFileIO&) = delete;
    AsyncFileIO& operator=(const AsyncFileIO&) = delete;
    AsyncFileIO(AsyncFileIO&&) = delete;
    AsyncFileIO& operator=(AsyncFileIO&&) = delete;

    /** AsyncFileIO 싱글톤 인스턴스를 반환합니다. */
    static AsyncFileIO& Get();

    /** AsyncFileIO 인스턴스가 초기화되어 있는지 확인합니다. */
    [[nodiscard]] static bool IsInitialized();

public:
    /**
     * 파일 전체를 비동기로 읽고, 완료 시 콜백을 Worker 스레드에서 호출합니다.
     *
     * @param path 읽을 파일의 경로 (null-terminated UTF-8 문자열)
     * @param callback I/O 완료 시 Compute Worker에서 호출될 콜백
     */
    void ReadFile(const Path& path, UniqueFunction<void(IOResult)>&& callback);

    /**
     * 파일 전체를 비동기로 읽어 코루틴으로 결과를 반환합니다.
     *
     * @param path 읽을 파일의 경로 (null-terminated UTF-8 문자열)
     * @return I/O 결과를 담은 JobTask. co_await로 대기합니다.
     */
    JobTask<IOResult> ReadFileAsync(const Path& path);

private:
    /** I/O 완료 큐를 감시하는 Poller Thread의 메인 루프 */
    void PollerLoop(const std::stop_token& stoken);

private:
    /** SDL 비동기 I/O 완료 큐 */
    SDL_AsyncIOQueue* io_queue = nullptr;

    /** I/O 완료 감시 전용 Poller Thread */
    std::jthread poller_thread;
};
} // namespace se
