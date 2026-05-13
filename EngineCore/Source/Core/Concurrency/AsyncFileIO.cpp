// ReSharper disable CppMemberFunctionMayBeConst
#include "SimpleEngine/Core/Concurrency/AsyncFileIO.h"
#include "SimpleEngine/Core/Concurrency/Coroutine/JobTask.h"
#include "SimpleEngine/Core/Concurrency/JobAllocator.h"
#include "SimpleEngine/Core/Concurrency/JobSystem.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/Platform.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Utility/Common.h"
#include "SimpleEngine/Utility/Debug.h"

#include <coroutine>


namespace se
{
namespace
{
/**
 * 비동기 I/O 요청의 컨텍스트
 *
 * SDL_AsyncIO에 전달하는 userdata로 사용되며,
 * Poller Thread가 완료를 감지했을 때 적절한 방식(콜백/코루틴)으로 결과를 전달합니다.
 */
struct IORequestContext
{
    enum class EMode : u8
    {
        Callback,  // UniqueFunction 콜백 호출
        Coroutine, // 코루틴 핸들 resume
    };

    /** 요청 모드 */
    EMode mode = EMode::Callback;

    /** 콜백 모드에서 I/O 완료 시 호출할 함수 */
    UniqueFunction<void(IOResult)> callback;

    /** 코루틴 모드에서 I/O 완료 시 재개할 코루틴 핸들 */
    std::coroutine_handle<> continuation;

    /** 코루틴 모드에서 결과를 저장할 위치 (코루틴 프레임 내부를 가리킴) */
    IOResult* result_storage = nullptr;

    // JobAllocator 기반 메모리 할당
    void* operator new(usize size) { return JobAllocator::Allocate(size); }
    void operator delete(void* ptr) { JobAllocator::Free(ptr); }
};


/**
 * 비동기 파일 읽기를 위한 코루틴 Awaitable
 *
 * await_suspend에서 SDL_LoadFileAsync를 호출하고,
 * Poller Thread가 완료를 감지하면 result에 데이터를 기록한 뒤 코루틴을 재개합니다.
 */
struct AsyncReadAwaitable
{
    SDL_AsyncIOQueue* queue;
    Path path;
    IOResult result; // 코루틴 프레임에 저장되어 suspension 동안 유효

    bool await_ready() const noexcept
    {
        return false;
    }

    bool await_suspend(std::coroutine_handle<> handle)
    {
        IORequestContext* ctx = new IORequestContext{};
        SE_SCOPE_DEFER_NAMED(ctx_delete) {
            delete ctx;
        };

        ctx->mode = IORequestContext::EMode::Coroutine;
        ctx->continuation = handle;
        ctx->result_storage = &result;

        if (!SDL_LoadFileAsync(path.CStr(), queue, ctx))
        {
            // SDL이 요청을 시작조차 하지 못한 경우, suspend하지 않고 즉시 복귀
            result.success = false;
            return false;
        }

        ctx_delete.Discard();
        return true;
    }

    IOResult await_resume()
    {
        return std::move(result);
    }
};


/**
 * SDL_AsyncIOOutcome으로부터 IOResult를 생성합니다.
 * SDL이 할당한 버퍼의 데이터를 Array<u8>로 복사한 뒤, SDL 버퍼를 해제합니다.
 */
IOResult BuildIOResult(SDL_AsyncIOOutcome&& outcome)
{
    SE_SCOPE_DEFER {
        if (outcome.buffer)
        {
            SDL_free(outcome.buffer);
            outcome.buffer = nullptr;
        }
    };

    IOResult result;
    result.success = outcome.result == SDL_ASYNCIO_COMPLETE;

    if (result.success && outcome.buffer && outcome.bytes_transferred > 0)
    {
        const usize size = static_cast<usize>(outcome.bytes_transferred);
        result.data.ResizeUninitialized(size);
        std::memcpy(result.data.Data(), outcome.buffer, size);
    }

    return result;
}
} // namespace


AsyncFileIO* AsyncFileIO::instance = nullptr;

AsyncFileIO::AsyncFileIO()
{
    SE_ASSERT(!instance, "AsyncFileIO instance already exists!");
    instance = this;

    io_queue = SDL_CreateAsyncIOQueue();
    SE_ASSERT(io_queue, "Failed to create SDL_AsyncIOQueue");

    poller_thread = std::jthread{ [this](const std::stop_token& stoken)
    {
        PollerLoop(stoken);
    }};

    ConsoleLog(ELogLevel::Info, "AsyncFileIO: Initialized");
}

AsyncFileIO::~AsyncFileIO()
{
    SE_ASSERT(instance == this, "AsyncFileIO instance mismatch!");

    // Poller Thread를 안전하게 종료
    if (poller_thread.joinable())
    {
        poller_thread.request_stop();

        // SDL WaitAsyncIOResult의 블로킹을 즉시 해제
        if (io_queue)
        {
            SDL_SignalAsyncIOQueue(io_queue);
        }

        poller_thread.join();
    }

    // SDL 큐 해제 (대기 중인 I/O는 SDL이 내부적으로 처리)
    if (io_queue)
    {
        SDL_DestroyAsyncIOQueue(io_queue);
        io_queue = nullptr;
    }

    instance = nullptr;
    ConsoleLog(ELogLevel::Info, "AsyncFileIO: Destroyed");
}

AsyncFileIO& AsyncFileIO::Get()
{
    SE_ASSERT(instance, "AsyncFileIO instance is not initialized!");
    return *instance;
}

bool AsyncFileIO::IsInitialized()
{
    return instance != nullptr;
}

void AsyncFileIO::ReadFile(const Path& path, UniqueFunction<void(IOResult)>&& callback)
{
    IORequestContext* ctx = new IORequestContext{};
    ctx->mode = IORequestContext::EMode::Callback;
    ctx->callback = std::move(callback);

    if (!SDL_LoadFileAsync(path.CStr(), io_queue, ctx))
    {
        // Load에 실패한 경우, 에러 결과를 Worker에서 전달 (Poller에서 직접 실행 금지)
        JobSystem::Get().Dispatch([ctx]
        {
            IOResult error_result;
            error_result.success = false;

            const UniqueFunction<void(IOResult)> cb = std::move(ctx->callback);
            delete ctx;
            cb(std::move(error_result));
        });
    }
}

JobTask<IOResult> AsyncFileIO::ReadFileAsync(const Path& path)
{
    co_return co_await AsyncReadAwaitable{
        .queue = io_queue,
        .path = path
    };
}

void AsyncFileIO::PollerLoop(const std::stop_token& stoken)
{
    Platform::SetCurrentThreadName("AsyncIO Poller");

    while (!stoken.stop_requested())
    {
        SDL_AsyncIOOutcome outcome{};

        // Signal이 올 때 까지 무한 대기
        if (!SDL_WaitAsyncIOResult(io_queue, &outcome, -1))
        {
            continue;
        }

        std::unique_ptr<IORequestContext> ctx{ static_cast<IORequestContext*>(outcome.userdata) };
        if (!ctx)
        {
            // userdata가 없는 결과는 무시 (SDL 내부 이벤트 등)
            if (outcome.buffer)
            {
                SDL_free(outcome.buffer);
            }
            continue;
        }

        // SDL 결과를 엔진 IOResult로 변환
        IOResult result = BuildIOResult(std::move(outcome)); // NOLINT(*-move-const-arg)

        // Context Mode에 따른 분기
        switch (ctx->mode)
        {
        case IORequestContext::EMode::Callback:
        {
            JobSystem::Get().Dispatch([safe_ctx = std::move(ctx), ret = std::move(result)] mutable
            {
                const auto cb = std::move(safe_ctx->callback);

                // 콜백을 실행하기 전에 컨텍스트 메모리를 먼저 해제
                safe_ctx.reset();

                cb(std::move(ret));
            });
            break;
        }

        case IORequestContext::EMode::Coroutine:
        {
            // 코루틴 프레임의 result_storage에 결과를 기록
            // 이후 JobSystem::Dispatch 의한 release/acquire가 happens-before를 보장
            *ctx->result_storage = std::move(result);
            const auto handle = ctx->continuation;

            // 콜백을 실행하기 전에 컨텍스트 메모리를 먼저 해제
            ctx.reset();

            // Worker 스레드에서 코루틴 재개
            JobSystem::Get().Dispatch([handle]
            {
                handle.resume();
            });
            break;
        }

        default:
            SE_UNREACHABLE();
        }
    }
}
} // namespace se
