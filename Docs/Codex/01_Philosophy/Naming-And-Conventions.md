---
작성일: 2026-05-07
최종 수정일: 2026-05-11
작성 완료: true
tags:
  - cpp
---
# 설계 철학 (Design Philosophy)

아래 네이밍 규칙은 두 가지 레퍼런스를 기반으로 설계했다.

**Unreal Engine에서 가져온 것:**

- 열거형 접두사 `E` (`EColor`, `EJobPriority`)
- 인터페이스 접두사 `I` (`IUpdatable`, `IMyInterface`)
- 괄호 스타일, 키워드 배치 순서 등 일부 문법 규칙

**Unreal Engine에서 버린 것:**

- `U`, `A`, `F` 등 타입 카테고리를 나타내는 접두사
  - 과도하고 이 프로젝트 규모에 맞지 않음

**Rust에서 가져온 것 (주된 영향):**

- 스코프나 종류를 접두사로 나타내지 않음: 이름은 오직 '의미'만 담음.
- `snake_case` 변수, `PascalCase` 타입, `UPPER_SNAKE_CASE` 컴파일 타임 상수의 명확한 구분
- `const`(런타임) vs `constexpr`(컴파일 타임) 대소문자 분리 규칙

---

## 1. 명명규칙

### 1.1. 타입 (Types)

- 클래스(`class`), 구조체(`struct`), 공용체(`union`), 타입 별칭(`using`, `typedef`), 콘셉트(`concept`)
  - 규칙: `PascalCase`
  - 예시: `MyClass`, `MyStruct`, `MyUnion`, `MyTypeAlias`

- 열거형(`enum`,`enum class`) 및 열거값
  - 규칙:
    - 열거형: `E` + `PascalCase`
    - 열거값: `PascalCase`
  - 만약 열거값이 여러줄인 경우 trailing comma를 권장

```cpp
enum class EColor
{
    Red,
    Green,
    Blue,
};
```

- 인터페이스(`class/struct`로 정의된 순수 가상 클래스)
  - 인터페이스의 정의
    - 순수 가상 함수만을 포함하는 클래스 (소멸자 제외)
    - 멤버 변수를 가지지 않는 클래스
    - 구현 코드를 포함하지 않는 클래스
  - 규칙: `I` + `PascalCase`
  - 예시: `IMyInterface`

- 추상 클래스
  - 추상 클래스의 정의
    - 하나 이상의 순수 가상 함수를 포함하는 클래스
    - 멤버 변수를 가질 수 있는 클래스
    - 구현 코드를 포함할 수 있는 클래스
  - 규칙: `PascalCase` + `Base` 권장 (선택 사항)
  - 예시: `MyAbstractClassBase`, `MyAbstractClass`

- 템플릿 매개변수
  - 규칙: 규칙: 타입 매개변수는 `T`, `U`, `PascalCase` + `Type` / 비타입 매개변수는 `UPPER_SNAKE_CASE`
  - 예시: `template <typename T>`, `template <int MAX_SIZE>`, `template <auto N>`

- 네임스페이스(`namespace`)
  - 규칙: `snake_case`, C++17의 중첩 네임스페이스를 적극 활용
  - 예시: `my_namespace`, `my_project::sub_module`

### 1.2. 함수 (Functions)

- 전역 함수, 멤버 함수, 정적 멤버 함수 모두 동일
- 규칙: `PascalCase` (동사 형태로 시작하는 것을 권장)
- 예시: `CalculateSum()`, `GetValue()`, `SetName()`

### 1.3. 변수 (Variables)

변수의 명명은 헝가리안 표기법이나 스코프(Scope)를 나타내는 접두사/접미사(`m_`, `s_`, `g_` 등)를 철저히 배제합니다. 오직 변수의 '의미'만으로 이름을 지으며, 불리언(`bool`) 타입에만 상태를 나타내는 접두사를 허용합니다.

- 예외적 허용: 불리언 (Boolean) 변수
  - 규칙: 상태나 조건을 묻는 `is_`, `has_`, `can_`, `should_` + `snake_case`
  - 예시: `is_active`, `has_weapon`, `can_jump`
  - 적용 범위: 아래 모든 변수 유형에 관계없이 `bool` 타입이라면 무조건 적용.

#### 1.3.1. 스코프 및 생명주기에 따른 규칙

모든 일반적인 변수는 `snake_case`를 사용합니다.

1. 지역 변수 (Local Variables) & 매개변수 (Parameters)
   - 규칙: `snake_case`, 매개변수의 경우 `in_`, `out_`, `inout_`등의 접두사 허용
   - 예시: `int player_score`, `float delta_time`, `std::string player_name`, `void SetPosition(float in_x, float in_y)`
2. 멤버 변수 (Member Variables)
   - 규칙: `snake_case`
   - 예시: `int health`, `std::string name`
3. 정적 멤버 변수 (지역 정적, 정적 멤버 변수)
   - 규칙: `snake_case`
   - 예시: `static int max_players`, `static float default_speed`
4. 전역 변수 (Global Variables)
   1. 익명 네임스페이스 내 변수 (.cpp 내부 전용, Internal Linkage)
      - 규칙: `snake_case`, 만약 지역 변수와 이름이 충돌할 가능성이 있다면 전역 스코프 지정자(`::`)를 사용하여 명확히 구분
      - 예시: `namespace { int game_state; }`, `game_state`, `::game_state`
   2. 공유 전역 변수 (Global Variables - External Linkage)
   - 규칙: `namespace global` + `snake_case` / `anonymous namespace` + `snake_case`
   - 예시: `namespace global { int my_score; }`, `global::my_score`
5. 정적 변수 (지역 정적, 정적 멤버)
   - 규칙: `snake_case`
   - 예시: `max_players`, `default_health`
6. 스레드 로컬 변수 (Thread-local Variables)
   - 규칙: `snake_case`
   - 예시: `thread_local int thread_id`, `thread_local std::string thread_name`

### 1.4. 상수 (Constants)

변수가 평가되는 시점(컴파일 타임 vs 런타임)에 따라 대소문자를 구분합니다.
변수의 스코프에 관계없이, 모든 상수는 해당 규칙을 따릅니다. (예: 지역 상수, 멤버 상수, 전역 상수 모두 동일한 규칙 적용)

1. 런타임 상수 (`const`)
   - 규칙: `snake_case` (일반 변수와 동일한 규칙 적용)
   - 예시: `const int max_players = 4;`, `const std::string game_title = "My Game";`
2. 컴파일 타임 상수 (`constexpr`, `constinit` 등)
   - 규칙: `UPPER_SNAKE_CASE`
   - 예시: `constexpr int MAX_PLAYERS = 4;`, `constexpr float PI = 3.14159f;`

```cpp
constexpr float MAX_MOVEMENT_SPEED = 500.0f; // 전역/네임스페이스

void CalculatePhysics()
{
    // 함수 내부에서만 사용되는 지역 constexpr도 대문자 사용
    constexpr float GRAVITY_CONSTANT = 9.81f;
    constexpr int32 MAX_ITERATIONS = 10;

    for (int32 i = 0; i < MAX_ITERATIONS; ++i)
    {
        // ...
    }
}
```

### 1.5. 매크로 (Macros)

- 규칙: `UPPER_SNAKE_CASE`
- 예시: `#define MAX_BUFFER_SIZE 1024`, `#define PI 3.14159f`
- 주의: 매크로는 가능한 한 사용을 피하고, 대신 `constexpr`이나 `inline` 함수를 사용하는 것을 권장합니다.

### 1.6. 파일 및 디렉토리 (Files and Directories)

- 파일 및 디렉토리 이름
  - 규칙: `PascalCase` + `.h` (헤더 파일), `PascalCase` + `.cpp` (소스 파일)
  - 예시: `MyClass.h`, `MyClass.cpp`

### 1.7. 헤더 가드

헤더 파일의 중복 포함 방지는 `#pragma once`만 사용합니다. (`#ifndef` 방식 금지)

### 1.8. 헤더 포함 (#include) 순서

헤더 포함은 아래의 4가지 그룹 순서를 따르며, **그룹 사이에는 빈 줄을 하나 둡니다.** 각 그룹 내부는 알파벳 순 정렬을 권장합니다.

```cpp
// 1. 대응하는 헤더 (.cpp 파일에서만 해당)
#include "SimpleEngine/Core/HAL/Platform.h"

// 2. 엔진 내부 헤더 (SimpleEngine/ or SimpleEditor/ prefix)
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"

// 3. 서드파티 라이브러리
#include "SDL3/SDL.h"
#include "tracy/Tracy.hpp"

// 4. 표준 라이브러리 (마지막)
#include <algorithm>
#include <memory>
```

## 2. 키워드 배치 순서

1. 변수 선언 순서
   - [연결성/정적] -> [평가 시점/상수] -> [타입] -> [식별자] -> [초기화]
   - 연결성 및 정적: `extern`, `static`, `thread_local`
   - 평가 시점 및 상수: `constinit`, `constexpr`, `const`
   - 타입: `int32`, `Vector3f` 등
   - 식별자: `health`, `MAX_SPEED` 등

   ```cpp
   static constexpr float MAX_SPEED = 300.0f;
   static constinit int32 active_instances = 0;
   int32 current_health = 100;
   ```

2. 함수 선언 순서
   - [매크로] -> [템플릿] -> [평가 시점/인라인] -> [가상화] -> [반환타입] -> [함수명(매개변수)] -> [const] -> [예외/조건] -> [오버라이드] -> [정의]
   - 평가 시점 / 인라인: `inline`, `constexpr`, `consteval`
   - 가상화 / 정적: `static`, `virtual`
   - 반환 타입: `void`, `int32`, `auto` 등
   - 함수명 및 매개변수: `DoSomething(int32 value)`
   - 상수 멤버 한정자: `const`
   - 예외 / 조건: `noexcept`, `requires`
   - 오버라이드: `override`, `final`
   - 정의 / 삭제: `= 0`, `= default`, `= delete`

   ```cpp
   // 예시 1: 일반적인 가상 함수 오버라이딩
   virtual void UpdateTick(float delta_time) const noexcept override;

   // 예시 2: C++20 Concept 적용 및 constexpr
   template <typename T>
   requires Integral<T>
   constexpr inline int32 CalculateMax(T a, T b) noexcept;

   // 예시 3: 인터페이스의 순수 가상 함수
   virtual void Draw() const = 0;
   ```

## 3. 포인터 및 레퍼런스 스타일

포인터(`*`)와 레퍼런스(`&`) 기호는 식별자(변수명)가 아닌 **타입(Type)**에 붙여서 작성합니다. 이는 변수보다 '타입 자체'를 강조하는 철학을 따릅니다.

- 권장 표기법 (Left-Aligned)

```cpp
int32* my_pointer;    // OK
const Vector& my_ref; // OK
```

- 금지 표기법 (C-Style)

```cpp
int32 *my_pointer;  // 금지 (C 스타일)
int32 * my_pointer; // 금지 (양쪽 공백)
```

## 4. 주석 및 문서화

- 문서 주석 (Public API): `/** */` Doxygen 스타일을 사용합니다. 짧은 설명은 한 줄로, 긴 설명은 여러 줄로 작성하며 `@brief` 키워드는 생략합니다.

```cpp
/**
 * 파일을 읽고 바이트 배열로 반환합니다.
 * @param file_path 읽을 파일의 경로
 * @return 성공 시 바이트 배열, 실패 시 에러를 담은 Expected
 */
[[nodiscard]] static FileResult<Array<uint8>> ReadBytes(const Path& file_path);
```

- 인라인 주석: 코드 우측이나 내부에 짧은 설명을 붙일 때는 `//`를 사용합니다.

TODO / FIXME: 주석과 함께 `SE_TODO()` 매크로를 사용하여 컴파일 시점에 경고로 출력되게 합니다.

```cpp
// TODO: C++26 컴파일러 나오면 기본 std 함수로 대체
SE_TODO("Replace with std function when C++26 is available")
```

## 5. 에러 처리 (Error Handling)

엔진 내부에서 `C++ 예외(exception)는 절대 사용하지 않습니다.` 상황에 따라 아래의 도구들을 적절히 선택합니다.

| 상황 | 도구 | 설명 |
| --- | --- | --- |
| 프로그래밍 오류 (절대 발생하면 안 됨) | SE_ASSERT | Debug/Dev 빌드에서만 검사. Release에서는 no-op. |
| 런타임 오류 (발생 가능성이 있음) | se::Expected<T, E> | 반환값으로 성공/실패를 명시적으로 전달. |
| 도달 불가 코드 | SE_UNREACHABLE() | Debug에서는 abort, Release에서는 std::unreachable(). |
| 미구현 코드 | SE_UNIMPLEMENTED() | Debug에서는 abort, Release에서는 no-op. |
| 복구 불가능한 치명적 오류 | SE_FATAL_ERROR(...) | 모든 빌드에서 std::terminate(). |

에러 처리 매크로 및 se::Expected 활용 예시:

```cpp
// 1. 단언문 (Assert)
SE_ASSERT(ptr != nullptr);        // 디버그 전용
SE_ASSERT(ptr != nullptr, "Pointer must not be null");        // 디버그 전용
SE_ASSERT_RELEASE(data != nullptr, "Critical data is null");  // 모든 빌드

// 2. se::Expected 모나딕 체이닝 패턴
ReadBytes(path)
    .AndThen([](Array<uint8>&& data) { return ParseData(std::move(data)); })
    .Map([](ParsedData&& parsed) { return parsed.vertices; })
    .OrElse([](const auto& err) { ConsoleLog(ELogLevel::Error, "{}", err.What()); });
```

## 6. 리플렉션 시스템 (Reflection System)

## 6.1. 클래스/구조체 등록

`SE_CLASS` 매크로는 클래스 내부 최상단(접근 지정자 앞)에 선언합니다.
리플렉션 프로퍼티 등록 코드(`SE_BEGIN_REFLECT` ~ `SE_END_REFLECT`)는 헤더가 아닌 `.cpp` 파일에 작성합니다.

```cpp
// 헤더 (MyComponent.h)
class SE_CORE_API MyComponent : public BaseComponent
{
    SE_CLASS(MyComponent, BaseComponent)

public:
    // ...
};

// 소스 (MyComponent.cpp)
namespace se
{
SE_BEGIN_REFLECT(MyComponent, meta::Reflect, meta::Component)
    SE_REFLECT_PROPERTY(health, meta::Property)
    SE_REFLECT_PROPERTY(speed,  meta::Property, meta::ReadOnly)
SE_END_REFLECT(MyComponent)
}
```

### 6.2. 어노테이션 (Annotation)

`SE_ANNOTATION`은 클래스/구조체 선언부, 또는 멤버 변수 바로 위 줄에 단독으로 위치시킵니다.
> 추후 C++26의 표준 어노테이션으로 쉽게 대체하기 위함

```cpp
struct SE_ANNOTATION(=meta::SerializeOnly) MySettings
{
    SE_ANNOTATION(=meta::Property)
    String level = "info";

    SE_ANNOTATION(=meta::Property, =meta::ReadOnly)
    uint32 version = 1;
};
```
