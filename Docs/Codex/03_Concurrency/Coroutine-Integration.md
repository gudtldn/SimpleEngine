---
작성일: 2026-05-20
최종 수정일: 2026-06-01
작성 완료: true
tags:
  - concurrency
  - coroutine
  - cpp20
  - async
  - cpp
---
# Coroutine 통합 - C++20 코루틴과 Job System의 결합

> **한 줄 요약:** 기존 C++20 코루틴 시스템을 갈아엎고, 엔진의 새로운 Work-Stealing Job System과 완전히 통합된 `JobTask<T>` 코루틴 타입과 API를 설계하여, 스레드 블로킹 없이 효율적인 비동기 작업 처리를 구현했다.

**코드 진입점:**

- `EngineCore/Include/SimpleEngine/Core/Concurrency/Coroutine/JobTask.h` - 코루틴 타입 및 promise_type
- `EngineCore/Include/SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h` - ResumeOn, WhenAll, WhenAny
- `EngineCore/Include/SimpleEngine/Core/Concurrency/JobHandle.h` - JobHandle::Awaiter (co_await 지원)
- `EngineCore/Include/SimpleEngine/Core/Concurrency/AsyncFileIO.h` - SDL3 기반 비동기 파일 I/O
- `EngineCore/Include/SimpleEngine/Core/Concurrency/JobAllocator.h` - 코루틴 프레임 Pool 할당

---

## 1. 도입 배경 및 설계 목표

Job System 리팩토링 이전에도 엔진에는 `Task<T>`와 `TaskScheduler`기반의 C++20 코루틴 시스템이 존재했다. 하지만 기존 시스템을 그대로 사용하기에는 두 가지 명확한 한계가 있었다.

1. **스레드 전환 API의 파편화:** 메인으로 갈 땐 `SwitchToMainThread`, 워커로 갈 땐 `SwitchToWorkerThread`처럼 전환할 때마다 각기 다른 API 이름을 사용하여 코드가 지저분해졌다.
2. **기반 시스템의 전면 교체:** 솔직히 코루틴 도입 당시에는 프레임의 힙 할당 문제나 내부 최적화 같은 깊은 로우레벨 지식은 없었다. 이번 개편의 가장 큰 이유는 기존 스케줄러(ThreadPool)를 폐기하고 Work-Stealing 기반의 Job System을 새로 구축하면서 구버전 코루틴 시스템이 더 이상 호환되지 않았기 때문이다.

기반 스케줄러가 바뀌어 어차피 코드를 수정해야 한다면, 억지로 땜질하는 대신 **새로운 Job System 인프라 위에 작동하는 래퍼(Wrapper)** 형태로 API를 바닥부터 다시 설계하기로 했다.

따라서 이번 새로운 코루틴 통합의 핵심 설계 목표는 **스레드 자체를 블로킹하지 않고 코루틴 프레임만 중단(Suspend)하여 워커 스레드가 유휴 시간 없이 다른 Job을 계속 처리하게 만드는 것** 이었다.

---

## 2. JobTask\<T\> - 코루틴 타입과 제어권 전환

> `EngineCore/Include/SimpleEngine/Core/Concurrency/Coroutine/JobTask.h`(`JobTask`, `JobTaskPromise`)

`JobTask<T>`가 엔진의 표준 코루틴 타입이며, 내부적으로 `JobTaskPromise<T>`가 생명주기를 관리한다.

```cpp
// 1. void 반환 코루틴
JobTask<void> LoadAssetAsync(Path path)
{
    IOResult result = co_await AsyncFileIO::Get().ReadFileAsync(path);
    co_await ResumeOn{ EJobThread::Main }; // 메인 스레드로 전환
    RegisterAsset(result.data);
}

// 2. 부모 코루틴에서 자식 코루틴 대기 (Symmetric Transfer)
JobTask<Mesh> ParseMeshAsync(Array<u8> raw_data)
{
    co_return ParseMesh(std::move(raw_data));
}

JobTask<void> ParentTask()
{
    Mesh mesh = co_await ParseMeshAsync(data); // 큐를 거치지 않고 직접 제어권 전환
    Upload(mesh);
}
```

### Lazy Start

`initial_suspend()`가 `suspend_always`를 반환하므로, `JobTask<T>` 객체를 생성해도 코루틴 본체는 즉시 실행되지 않는다. `SubmitTask()` 또는 `co_await`을 통해 Job System이 명시적으로 `resume()`할 때 비로소 실행이 시작된다.

### Symmetric Transfer (Zero-Cost `co_await`)

`co_await child_task`를 실행하면 Job System 큐를 거치지 않고 자식 코루틴으로 직접 제어가 넘어간다. 꼬리 호출(tail call) 방식이라 스택도 새로 쌓지 않는다. 완료되면 `FinalAwaiter`가 부모로 돌려보낸다.

```mermaid
graph LR
    P["부모 코루틴"] -->|"co_await child"| C["자식 코루틴"]
    C -->|"완료 시 FinalAwaiter"| P

    style P fill:#e3f2fd,stroke:#1565c0,stroke-width:2px
    style C fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px
```

### Detached 모드 (SubmitTask / DispatchTask)

`SubmitTask()`나 `DispatchTask()`로 제출하면 소유권이 Job System으로 넘어가고, 완료 후 `FinalAwaiter`가 프레임을 자동 소멸시킨다. `SubmitTask()`는 추가로 `JobHandle`에 완료를 통지한다.

| 제출 방식 | 완료 추적 | 프레임 수명 |
| --- | :---: | --- |
| `DispatchTask` | X | `FinalAwaiter`가 자동 소멸 |
| `SubmitTask` | O (`JobHandle`) | `FinalAwaiter`가 자동 소멸 후 카운터 통지 |
| `co_await task` | 부모가 암묵적 추적 | 부모가 소유, 완료 후 부모에서 소멸 |

---

## 3. 캡처 금지 설계 (Dangling Reference 원천 차단)

기존 개발 과정에서 가장 빈번했던 버그는 코루틴이 Suspend된 사이 람다 객체가 소멸하여 발생하는 **캡처 변수 Dangling Reference** 문제였다. 이를 구조적으로 제거하기 위해 `JobTaskPromise`에 컴파일 타임 방어막을 세웠다.

```cpp
// 잘못된 사용: 캡처 람다 사용 (컴파일 타임에 static_assert 발생)
auto task = [path]() -> JobTask<void>
{
    co_await AsyncFileIO::Get().ReadFileAsync(path);
};
JobSystem::Get().DispatchTask(task); // 컴파일 에러

// 올바른 사용: 캡처 불가(static 람다) + 파라미터로 값 명시적 전달
JobSystem::Get().DispatchTask([](Path p) static -> JobTask<void>
{
    co_await AsyncFileIO::Get().ReadFileAsync(p);
}(path)); // 람다를 즉시 호출(IIFE)하여 반환된 JobTask를 제출
```

캡처(`[...]`)가 들어간 람다를 코루틴으로 쓰려 하면 컴파일이 거부된다. 캡처를 비우고(`[]`) 반드시 `static` 람다를 사용하게 강제하여 메모리 오염 가능성을 원천 차단했다.

---

## 4. 코루틴 프리미티브 API

> `EngineCore/Include/SimpleEngine/Core/Concurrency/Coroutine/CoroutinePrimitives.h`

### 명시적 스레드 전환 (`ResumeOn`)

```cpp
co_await ResumeOn{ EJobThread::Main };   // 이후 코드를 메인 스레드에서 실행
co_await ResumeOn{ EJobThread::Worker }; // 이후 코드를 워커 스레드에서 실행
```

`co_await child_task`가 큐를 우회한다면, `ResumeOn`은 현재 코루틴을 Job System 큐에 재삽입하여 스레드를 전환하는 **유일한 경로**다. "워커에서 연산 -> 메인에서 UI 갱신" 패턴같이 콜백 지옥으로 빠지기 쉬운 시나리오에서 매우 유용하다.

### 복수 핸들 대기 (`WhenAll` / `WhenAny`)

```cpp
auto handle_a = JobSystem::Get().Submit([]{ StageA(); });
auto handle_b = JobSystem::Get().Submit([]{ StageB(); });

// WhenAll: a와 b 모두 완료될 때까지 대기
co_await WhenAll{ handle_a, handle_b };
FinalStage();

// WhenAny: a 또는 b 중 하나라도 완료되면 재개
co_await WhenAny{ handle_a, handle_b };
HandleEarliest();
```

`WhenAll`은 Guard-Count 패턴으로 각 핸들에 콜백을 등록해 마지막에 코루틴을 깨우고, `WhenAny`는 첫 번째 완료 신호만 코루틴 재개를 트리거하도록 설계했다.

### JobHandle `co_await` 지원

`JobHandle::operator co_await()`가 `Awaiter`를 반환하므로, 코루틴 내에서 일반 `JobHandle`을 직접 대기할 수 있다.

```cpp
JobTask<void> WaitForOtherJob()
{
    JobHandle handle = JobSystem::Get().Submit([]{ DoWork(); });
    co_await handle; // handle 완료 시 재개
    UseResult();
}
```

---

## 5. 코루틴 프레임 힙 할당 최적화

C++20 코루틴의 고질적인 단점은 프레임 생성 시 매번 힙 할당(`operator new`)이 일어난다는 점이다.

이를 해결하기 위해 `JobTaskPromise<T>`의 `operator new/delete`를 직접 오버로딩하여, Job System의 **Lock-Free TLS 할당자(`JobAllocator`)에 직접 위임**했다. 그 결과, `malloc`을 호출하는 대신 스레드 로컬에 준비된 64/128/256/512 바이트 버킷에서 메모리를 즉시 꺼내 쓰도록 만들어 힙 파편화와 락 경합을 제거했다.

---

## 6. 논블로킹 I/O 연동(`AsyncFileIO`)

> `EngineCore/Include/SimpleEngine/Core/Concurrency/AsyncFileIO.h`

SDL3의 `SDL_AsyncIO`를 사용하여 OS 수준의 비동기 I/O를 지원한다. 전용 Poller Thread가 `SDL_AsyncIOQueue`를 감시하다가 완료 이벤트가 들어오면, 워커 스레드에 콜백이나 코루틴 재개(Resume)를 스케줄링한다.

### 6.1. IOResult Zero-Copy 개선 (PR [#43](https://github.com/gudtldn/SimpleEngine/pull/43))

`BuildIOResult`의 초기 구현에서는, SDL이 할당한 버퍼를 엔진 소유의 `Array<u8>`로 `memcpy`한 뒤 SDL 버퍼를 즉시 해제하는 구조였다. 이렇게 되면서 매번 비동기 I/O를 완료할 때마다 파일 크기만큼 복사가 발생하는 비효율적인 상황이 벌어졌다.

```cpp
// 기존: SDL 버퍼 -> Array<u8> 복사 후 SDL_free
result.data.ResizeUninitialized(size);
std::memcpy(result.data.Data(), outcome.buffer, size);
SDL_free(outcome.buffer);
```

이제는 SDL 버퍼의 소유권을 `IOResult`로 이전하여, `IOResult`가 소멸될 때 자동으로 `SDL_free`가 호출되도록 변경했다.

```cpp
// 변경: std::exchange로 소유권 이전 (Zero-Copy)
result.data_ptr.reset(static_cast<u8*>(std::exchange(outcome.buffer, nullptr)));
result.data_len = static_cast<usize>(outcome.bytes_transferred);
```

### 6.2. `Success()`의 의미 변경

기존 `bool success` 필드는 SDL의 `ASYNCIO_COMPLETE` 여부, 즉 순수한 I/O 완료를 나타냈다. 변경 후 `Success()`는 `data_ptr != nullptr`를 반환하므로 **1바이트 이상의 데이터를 수신했는지**를 뜻한다. 에셋 파이프라인에서 빈 파일은 항상 무효 데이터이므로 "I/O 완료지만 데이터 없음"을 별도로 구분할 실익이 없어 두 케이스를 통합했다.

```cpp
// 콜백 방식
AsyncFileIO::Get().ReadFile(path, [](IOResult result)
{
    if (result.Success())
    {
        ProcessData(result.AsView());
    }
});

// 코루틴 방식
JobTask<void> LoadAssetTask(Path path)
{
    IOResult result = co_await AsyncFileIO::Get().ReadFileAsync(path);
    if (result.Success())
    {
        ProcessData(result.AsView());
    }
}
```

---

## 7. 한계 및 미래 과제

- **예외 미지원:** `unhandled_exception()`에서 `SE_FATAL_ERROR()`로 즉시 크래시한다. 안정성을 위해 예외 전파를 의도적으로 막았으나, 에러 결과를 `co_return`으로 넘기는 `Expected<T, E>` 패턴 도입을 검토 중이다.
- **취소(Cancellation) 미지원:** 제출된 코루틴 대기열을 외부에서 중단하는 메커니즘이 없다. 추후 `stop_token`기반 취소 지원을 추가할 계획이다.

---

## 참고

- [비동기 파일 I/O 시스템 및 코루틴 안전성 강화 PR #20](https://github.com/gudtldn/SimpleEngine/pull/20)
- [IOResult Zero-Copy 개선 PR #43](https://github.com/gudtldn/SimpleEngine/pull/43)
- [Job-System-Architecture.md](./Job-System-Architecture.md) - WorkStealingDeque 및 JobSystem 스케줄러 상세
- [C++20 Coroutine Reference (cppreference)](https://en.cppreference.com/w/cpp/language/coroutines)
- [Symmetric Transfer (Lewis Baker)](https://lewissbaker.github.io/2020/05/11/understanding_symmetric_transfer)
