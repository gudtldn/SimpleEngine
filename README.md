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

- module export/import 키워드 순서

> example.cppm

```c++
module;
// 전역 프래그먼트
export module SimpleEngine.Core:ExamplePartition;

export import :TestPartition1;
export import :TestPartition2;

import :OtherPartition;
import SimpleEngine.AnotherModule;

import std;
import <header.h>;
```

> example.cpp

```c++
module;
// 전역 프래그먼트
module SimpleEngine.Core;
import :ExamplePartition;

import :OtherPartition;
import SimpleEngine.AnotherModule;

import std;
import <header.h>;
```

### 프로젝트 구조 및 명명 규칙

**1. 폴더 기반 모듈 (Folder-based Modules)**

- `Source` 디렉터리 아래의 각 기능 폴더(`PascalCase`)는 하나의 독립된 논리적 모듈을 구성합니다.
- **예시:** `Source/Utility/` 폴더는 `Utility` 기능과 관련된 모든 코드를 담는 `SimpleEngine.Utility` 모듈이 됩니다.

**2. 모듈 명명 규칙 (Module Naming)**

- **형식:** `SimpleEngine.Category:ModuleName`
- 모듈 이름은 `SimpleEngine` 접두사, 상위 카테고리(e.g., `Core`, `Subsystems`), 그리고 해당 모듈의 기능명(폴더명)으로 구성됩니다.
- **예시:**
    - `Source/Utility/` 폴더 -> `SimpleEngine.Utility` 모듈
    - `Source/Core/Interface/` 폴더 -> `SimpleEngine.Core:Interface` 모듈

> 만약 순환 종속성이 발생하게 된다면, 파티션으로 나누지 않아도 됨.

**3. 파일 구조 규칙 (File Structure)**

- 각 모듈 폴더는 폴더명과 동일한 이름의 **주 모듈 인터페이스 파일**을 가집니다. 이 파일은 모듈의 진입점 역할을 합니다.
    - **예시:** `Source/Utility/` 폴더 안에는 `Utility.cppm` 파일이 존재하며, 이 파일이 `export module SimpleEngine.Utility;`를 선언합니다.
- 모듈에 속한 개별 기능 파일들은 **모듈 파티션(Partition)**으로 작성됩니다.
    - **예시:** `Source/Utility/StringUtils.cppm` 파일은 `export module SimpleEngine.Utility:StringUtils;` 와 같이 자신을 `Utility`
      모듈의 파티션으로 선언합니다.
- 주 모듈 인터페이스 파일(`Utility.cppm`)은 내부 파티션들을 `export import` 하여 모듈의 공개 API를 결정합니다.
  ```cpp
  // In: Source/Utility/Utility.cppm
  export module SimpleEngine.Utility;

  export import :StringUtils; // StringUtils 파티션을 외부에 공개
  ```

**4. 네임스페이스 규칙 (Namespace)**

- **형식:** `se::category::module_name` (`snake_case`)
- 네임스페이스는 `se` 접두사와 모듈의 경로를 `snake_case`로 변환하여 사용합니다.
- **예시:**
    - `SimpleEngine.Utility:StringUtils` 모듈 -> `namespace se::utility::string_utils`
    - `SimpleEngine.Core:StringName` 모듈 -> `namespace se::core::string_name`
- **예외:** 엔진의 가장 기본적인 타입을 정의하는 `SimpleEngine.Types` 모듈의 `CoreTypes` 파티션은 전역적인 사용성을 위해 네임스페이스를 사용하지 않습니다.
- ConsoleLog도 포함

### 참고한 프로젝트 및 엔진
- [Wicked Engine](https://github.com/turanszkij/WickedEngine)
- [Unreal Engine](https://github.com/EpicGames/UnrealEngine)
- [Bevy Engine](https://github.com/bevyengine/bevy)

</details>
