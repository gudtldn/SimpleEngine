# SimpleEngine

```shell
git clone --recursive https://github.com/gudtldn/SimpleEngine.git
```

```shell
git submodule update --init --recursive
```

<details>

### 추후에 문서에 정리
- C++ latest 프로젝트 (C++26 ~ 29 까지 보고있음)
- 기존의 헤더 방식이 아닌, C++20의 모듈을 사용
- 핵심 구조는 OOP, 게임 로직은 ECS
- Lua, Pyhton? 지원 예정
- Visual Sctipt도 만?들 예정
- 프로젝트의 기본 string타입은 char8_t, std::u8string 타입

#### 코드 컨벤션 (수정될 수 있음)
- PascalCase
  - 함수명: `void TestFunction();`
  - 클래스명: `class TestClass;`
  - static 변수명 `static int TestVariable;`
  - Enum(접두사 E), Enum의 열거자 `enum class EMyEnum;`
  - 모듈명: `module TestModule;`
- snake_case
  - static 변수를 제외한 모든 변수명 (매개변수, 멤버변수 포함)
    - `int my_variable;`
    - `int in_my_variable;` 이름이 겹치는 매개변수는 `in/out`을 접두사로 사용
  - namespace: `namespace se::my_namespace {}`
- ALL_UPPER
  - 상수
- 공백 4칸을 사용
- 중괄호는 항상 있어야 하며, BSD 스타일을 따름
```c++
// X
if (expr) return true;
if (expr)
    return true;
if (expr) {
    return true;
}

// O
if (expr)
{
    return true;
}
```

### 참고한 프로젝트 및 엔진
- [Wicked Engine](https://github.com/turanszkij/WickedEngine)
- [Unreal Engine](https://github.com/EpicGames/UnrealEngine)
- [Bevy Engine](https://github.com/bevyengine/bevy)

</details>
