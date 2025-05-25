# SimpleEngine

```shell
git clone --recursive https://github.com/gudtldn/SimpleEngine.git
```

```shell
git submodule update --init --recursive
```

### 추후에 문서에 정리
- C++ latest 프로젝트 (C++26 ~ 29 까지 보고있음)
- 기존의 헤더 방식이 아닌, C++20의 모듈을 사용
- 프로젝트의 기본 string타입은 char8_t, std::u8string 타입

#### 코드 컨벤션 (수정될 수 있음)
- PascalCase
  - 함수명
  - 클래스명
  - static 변수명
  - Enum(접두사 E), Enum의 열거자
  - 모듈명
- snake_case
  - static 변수를 제외한 모든 변수명 (매개변수, 멤버변수 포함)
  - namespace
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
