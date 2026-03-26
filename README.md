# SimpleEngine

```shell
git clone https://github.com/gudtldn/SimpleEngine.git
git submodule update --init
```

## 빌드 환경

| 도구                        | 버전     | 비고                      |
|---------------------------|--------|-------------------------|
| CMake                     | 3.28+  |                         |
| vcpkg                     | 최신     | `VCPKG_ROOT` 환경변수 설정 필요 |
| Rust / rustup             | stable | ICU4X Corrosion 빌드에 필요  |
| MSVC (Visual Studio 2022) | 17.x+  |                         |
| Ninja (필수는 아님)            | 최신     | 단일 구성 생성기 사용 시 필요       |

좌표계
right-hand, z-up

<details>

## 추후에 문서에 정리

- C++ latest 프로젝트 (C++26 ~ 29 까지 보고있음)
- ~~기존의 헤더 방식이 아닌, C++20의 모듈을 사용~~
  - 모듈 불편한게 한두가지가 아니라 다시 헤더로 변경, 추후에 기회가 될 때 모듈로 변경
  - 순환참조가 생각보다 빈번하게 발생하고, 전방 선언이 잘 안되는 경우가 많았음
- 핵심 구조는 OOP, 게임 로직은 ECS
- Lua, Pyhton? 지원 예정
- Visual Sctipt도 만?들 예정
- ~~프로젝트의 기본 string타입은 char8_t, std::u8string 타입~~
  - char8_t가 기존 std나 라이브러리 코드와 호환이 잘 안되고, 컴파일러 설정에 utf-8을 적용시키면 일반 char로도 유니코드 사용이 가능해서 다시 변경

### 코드 컨벤션 (수정될 수 있음)

#### Naming Conventions

1. 파일명 (File Names): PascalCase
    - `MyActor.h`, `RenderData.cpp`
2. 타입 및 인터페이스 (Types): PascalCase
    - `class MyActor`, `struct RenderData`
    - Interface는 `I` 접두사 사용: `class IRenderable`
    - Abstract 클래스는 접미사 `~Base` 사용을 권장 (선택 사항): `class ActorBase`
3. 템플릿 (Templates):
    - 타입 템플릿 파라미터:
        - 단순 타입: `T`, `U`, `K`, `V` 등등
        - 의미가 있는 타입: PascalCase (예: `typename KeyType`, `typename ValueType`)
    - 비타입 템플릿 파라미터(NTTP): `UPPER_SNAKE_CASE`
        - `template <typename T, int32 MAX_SIZE>`, `template <typename T, auto ALIGNMENT>`
4. Enum: E + PascalCase, 내부 요소는 PascalCase (scoped enum 사용 권장)
    - `enum class ERenderMode`, `enum class EInputAction`
    - `ERenderMode::Wireframe`, `EInputAction::Press`
5. 변수 (Variables)
    1. 전역 (Global) / 네임스페이스 범위
        - 전역 변수 / `thread_local`
            - 변경이 가능한 전역 변수는 최대한 피하는 것을 권장
            - 필요한 경우 `static`키워드 대신 `anonymous namespace`을 사용
            - 만약 외부 공개가 필요하다면 `namespace global`을 사용
            - 이름: PascalCase
            - 예: `namespace { int32 GlobalVariable = 0; }`
        - `constexpr` / `static constexpr`:
            - 규칙: `constexpr`만 사용. `static`이나 `inline`은 굳이 붙이지 않음
            - 이름: `UPPER_SNAKE_CASE`
            - 예: `constexpr int32 MAX_BUFFER_SIZE = 1024;`
        - `const` / `static const`
            - 규칙: `const`만 사용. `static`은 사용하지 않음
            - 이름: `UPPER_SNAKE_CASE`
            - 예: `const int32 PROCESS_ID = GetPID();`
    2. 지역 (Local) 범위
        - 최대한 맥락에 맞는 이름 사용 권장 (예: `position`, `velocity`, `color` 등)
        - bool: `is_`, `has_`, `can_` 접두사 사용 권장 (예: `is_visible`, `has_texture`, `can_jump`)
        - 매개변수:
            - 매개변수에서 이름이 겹칠 때, `in_`, `out_` 접두사 사용 권장
            - 이름: `snake_case`
            - 예: `void Foo(int32 in_value, int32& out_value);`
        - `constexpr` / `static constexpr`:
            - 규칙: 함수 내에서만 쓰이는 컴파일 타임 상수가 필요할 때 사용
            - 이름: `UPPER_SNAKE_CASE` (상수임을 명시)
            - 예: `static constexpr int32 MAX_RETRY = 5;`
        - `const`:
            - 규칙: 재할당이 없는 모든 변수에 적극 사용 권장
            - 이름: `snake_case`
            - 예: `const int32 current_idx = get_index();`
        - `static` / `static const`:
            - 규칙: 상태 유지가 필요한 경우만 사용. (Thread-safe 고려 필요)
            - 이름: `snake_case`
            - 예: `static int32 call_count = 0;`
        - `thread_local`:
            - 규칙: 스레드별 독립적인 상태가 필요할 때 사용
            - 이름: `snake_case`
            - 예: `thread_local int32 thread_hit_count = 0;`
    3. 멤버 (Member) 범위 (Class/Struct)
        - 일반 멤버 변수
            - 규칙: `snake_case`. 접두사(m_)나 접미사(_) 절대 금지.
        - `static constexpr`:
            - 규칙: 클래스 수준의 상수는 무조건 이 형식을 사용.
            - 이름: `UPPER_SNAKE_CASE`
            - 예: `static constexpr float PI = 3.141592f;`
        - `static const`:
            - 규칙: constexpr로 표현할 수 없는 런타임 클래스 상수일 때만 제한적으로 사용.
            - 이름: UPPER_SNAKE_CASE
        - `static` 멤버:
            - 규칙: 클래스 변수(Class Variable)로서 모든 인스턴스가 공유함.
            - 이름: `snake_case` (접두사/접미사 금지)
            - 접근: 클래스 내부에서도 명확성을 위해 `ClassName::variable_name`으로 접근하는 것을 권장 (Shadowing 방지 및 공유 변수임을 명시).
        - `static thread_local` 멤버
            - 규칙: 클래스 내 스레드별 공유 상태가 필요할 때 사용.
            - 이름: `snake_case`
6. 함수 (Functions): PascalCase
    - 가상 함수 사용시, 파생 클래스에서도 `virtual ~ override` 키워드 필수
    - `[[nodiscard]]` 사용: 리턴값을 무시하면 안 되는 함수(예: IsEmpty(), CreateInstance())에 적극 사용.
    - `const` 멤버 함수: 상태를 바꾸지 않는 함수는 무조건 뒤에 const 명시. (상황에 따라서 생략 가능)
7. Namespace: `snake_case`
    - `namespace se`, `namespace se::graphics`
    - C++17의 nested namespace 사용 권장: `namespace se::graphics`
    - 내부 구현은 `detail` namespace 사용: `namespace se::detail`
    - namespace내부 코드는 들여쓰지 않는 것을 권장
    - namespace 끝에는 `// namespace se::graphics`와 같이 주석으로 닫는 것을 권장

클래스 체계

```cpp
class EXAMPLE_API SE_ANNOTATION(=meta::Reflect) MyClass : public BaseClass
{
    SE_CLASS(MyClass, BaseClass)

public:
    MyClass();
    ~MyClass();

private:
    SE_ANNOTATION(=meta::Property)
    int my_variable;
};
```

> SE_ANNOTATION은 C++26으로 마이그레이션을 편하게 하기 위한 매크로입니다. C++26에서는 [[=meta::Reflect]]와 같은 형태로 어노테이션을 사용할 수 있습니다.

### 프로젝트 구조 및 명명 규칙

### 참고한 프로젝트 및 엔진

- [Wicked Engine](https://github.com/turanszkij/WickedEngine)
- [Unreal Engine](https://github.com/EpicGames/UnrealEngine)
- [Bevy Engine](https://github.com/bevyengine/bevy)

</details>
