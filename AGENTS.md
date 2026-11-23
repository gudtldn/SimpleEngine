# AGENTS.md

SDL3GPU기반 C++23 게임 엔진입니다. 엔진 코어는 OOP, 게임 로직은 ECS로 작성합니다.
좌표계는 right-hand, z-up이고 1 unit = 1 m입니다.

## 명령어

Windows + MSVC에서만 빌드·테스트가 검증됐습니다 (x64 Native Tools 프롬프트, `VCPKG_ROOT` 필요).

```shell
git submodule update --init --recursive
cmake --preset debug-msvc
cmake --build --preset debug-msvc

# 테스트는 반드시 실행 파일 폴더에서 실행 (현재 디렉터리의 Config/를 마운트함)
cd Binaries/Windows/x64/Debug
./UnitTests.exe --gtest_filter=PathTest.*
```

- `ctest`에는 테스트가 등록돼 있지 않습니다. `UnitTests.exe`를 직접 실행합니다.
- 빌드하거나 테스트를 돌리지 못했다면 결과에 "빌드·테스트 미검증"이라고 적습니다. 돌리지 않은 테스트를 통과했다고 말하지 않습니다.

## 하지 말 것

- `ThirdParty/`(git 서브모듈)는 수정하지 않습니다.
- 예외를 쓰지 않습니다. 실패할 수 있는 작업은 `se::Expected`, 프로그래밍 오류는 `SE_ASSERT`로 처리합니다.
- 실패하는 테스트를 지우거나 기대값(골든 파일 포함)을 바꿔서 통과시키지 않습니다. 의도한 동작 변경일 때만 바꿉니다.

## 코드 규칙

전체 규칙은 `Docs/Codex/01_Philosophy/Naming-And-Conventions.md`에 있습니다. 아래는 자주 틀리는 것만입니다.

- `bool`은 항상 `is_`/`has_`/`can_`/`should_`로 시작합니다.
- `constexpr`·`constinit` 상수는 `UPPER_SNAKE_CASE`, 런타임 `const`는 `snake_case`입니다.
- enum은 `E`, 인터페이스는 `I` 접두사를 붙입니다.
- 정수·실수는 `i32`, `u64`, `f32`, `usize` 같은 엔진 별칭을 씁니다 (`int32`는 없음).
- 컨테이너는 `Array`, `String`, `Optional`, `HashMap` 같은 엔진 것을 씁니다. 표준 컨테이너는 서드파티 API 경계에서만 씁니다.
- 주석은 한국어로 씁니다. TODO는 주석과 함께 `SE_TODO("...")`를 씁니다.
- 모듈 밖에서 쓰는 클래스·함수에는 export 매크로(`SE_CORE_API`, `SE_EDITOR_API`)를 붙입니다. 빠뜨리면 링크 에러가 납니다.

## 테스트

기존 테스트 스타일이 아니라 아래 규칙을 따릅니다. 기존 위반은 그 파일을 고칠 때만 정리합니다.

- 바꾼 동작을 검증하는 데 필요한 만큼만 씁니다. 입력만 바꾼 반복, `= default` 함수 검증, 다른 테스트와 겹치는 검증은 쓰지 않습니다.
- 모든 테스트는 실패할 수 있어야 합니다. 단언 없는 테스트나 항상 참인 검사는 쓰지 않습니다.
- 테스트 이름은 `_` 없는 PascalCase 문장입니다 (`PopOnEmptyReturnsNone`).
- 테스트용 타입·헬퍼는 파일별 네임스페이스에 둡니다 (`FooBarTest.cpp` → `se_foo_bar_test`). 모든 테스트가 한 실행 파일로 링크되므로 전역에 두면 이름이 충돌합니다.
- 뒤에서 값을 꺼내 쓰는 전제 조건은 `ASSERT_*`로 검사합니다. `EXPECT_*`면 실패 뒤 빈 값에 접근해 테스트 프로세스 전체가 죽습니다.
- `EXPECT_THROW`·`EXPECT_NO_THROW`는 쓰지 않고, 컴파일 타임 검증은 네임스페이스 범위 `static_assert`로 둡니다.
- 픽스처는 SetUp/TearDown이나 공유 상태가 있을 때만 씁니다.

## 작업 방식

- 커밋 메시지는 `<type>: <한국어 요약>`입니다. type은 `feat`, `refactor`, `test`, `docs`, `style`, `chore`, `build`, `note` 중 하나입니다.
- 서브시스템 설계를 바꾸면 관련 문서도 고치고 front matter의 `최종 수정일`을 갱신합니다.

## 참고 문서

- `Docs/Codex/00_Meta/QuickStart.md`: 빌드 환경, 프리셋 목록, 문제 해결
- `Docs/Codex/01_Philosophy/Naming-And-Conventions.md`: 명명·서식·리플렉션 등록 규칙
- `Docs/Codex/` 하위 폴더: 서브시스템별 설계 문서. 해당 서브시스템을 고치기 전에 읽습니다.
