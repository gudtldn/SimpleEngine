---
작성일: 2026-05-11
최종 수정일: 2026-05-13
작성 완료: true
tags:
  - foundation
  - unicode
  - build
---
# 유니코드 라이브러리를 세 번 바꾼 이유 (utf8cpp -> ICU4C -> ICU4X)

> **한 줄 요약:** 단순 기능으로는 `utf8cpp`로도 충분했지만, 제대로 된 국제화(i18n)를 지원하려다 `ICU4C`의 바이너리 비대화와 UTF-16 오버헤드에 데인 후, 최종적으로 Rust 기반의 UTF-8 네이티브인 `ICU4X`에 정착했다.

**Code Entry Point** - 이 결정의 흔적을 보려면 여기부터:
- `EngineCore/Source/Core/Container/String.cpp` - ICU4X `CaseMapper` 실제 사용부
- `Tools/CMake/SEDependencies.cmake` - Corrosion + icu4x_src FetchContent 설정
- `EngineCore/CMakeLists.txt` - ICU4X C++ FFI 바인딩 헤더 include path 설정

---
## 1. 왜 유니코드 라이브러리를 세 번이나 바꿨는지?

엔진의 `String` 타입은 `char` 기반 UTF-8 문자열이다. 사실 기본적인 인코딩 검증이나 단순 처리만 놓고 보면 `utf8cpp` 라이브러리로도 충분했다.

하지만 "이왕 자체 엔진을 만드는 거, 국제화(i18n)와 제대로 된 유니코드(대소문자 변환, 정규화 등)까지 완벽하게 지원해 보자"는 욕심이 생겼다. 단순한 바이트 처리로는 언어권(Locale)에 맞는 올바른 문자열 연산을 구현할 수 없었기 때문이다. 이로 인해서 프로젝트 초창기부터 현재까지 라이브러리가 여러번 바뀌는 상황이 발생했다.

---
## 2. 마이그레이션 타임라인

| 단계                      | 라이브러리                             | 탈락 이유                                                          |
| ----------------------- | --------------------------------- | -------------------------------------------------------------- |
| **1st - utf8cpp**       | 경량, 헤더 온리                         | 정규화(Normalization), 대소문자 변환 등 고급 기능 부재. 단순 UTF-8 인코딩 검증 수준에 그침 |
| **2nd - ICU4C**         | 업계 표준, 기능 완비                      | 바이너리 비대 + 내부 UTF-16 기본 사용 -> UTF-8 변환 오버헤드                     |
| **-> 3rd - ICU4X (채택)** | Rust 기반 경량, UTF-8 네이티브, 기능별 선택 가능 | Corrosion 빌드 통합 복잡도 및 Rust 컴파일러 필요 (수용)                        |

---
## 3. ICU4C 도입 후 버린 이유

업계 표준인 ICU4C를 과감히 버린 구체적인 이유는 다음과 같다.

1. **배보다 배꼽이 더 큰 바이너리**: ICU4C는 유니코드 처리의 모든 것을 담고 있는 거대한 라이브러리다. 엔진에서 실제로 필요한 기능은 **대소문자 변환과 일부 문자열 처리뿐**인데, 이 기능을 쓰자고 전체 ICU 스택을 끌고 오니 빌드 시간이 크게 늘어났고 바이너리 용량도 납득할 수 없는 수준으로 비대해졌다.

2. **UTF-16 오버헤드**: 언리얼 엔진이 `TCHAR`(UTF-16)를 쓰면서 겪는 고통과 유사한 문제가 발생했다. ICU4C의 핵심 타입인 `UnicodeString`은 내부적으로 UTF-16을 사용한다. 따라서 엔진의 네이티브 `String`(UTF-8)을 ICU에 넘겨 처리하려면, 모든 연산마다 양방향 인코딩 변환 비용을 치러야만 했다. `(UTF-8) -> (UTF-16) -> [ICU 처리] -> (UTF-16) -> (UTF-8)`

---
## 4. ICU4X를 최종 선택한 이유와 통합 과정

### 4.1. 선택한 이유

- **UTF-8 네이티브**: Rust의 `&str`를 직접 받아 처리하므로 인코딩 변환 오버헤드가 없다.
- **기능별 분리 (모듈화)**: 필요한 컴포넌트만 Cargo feature로 선택해 바이너리에 포함할 수 있어서 비대화 문제가 완전히 해결되었다.
- **C++ FFI 바인딩 제공**: C++ 코드에서 직접 호출이 가능한 헤더를 제공했다.

### 4.2. CMake 통합 (Corrosion)

Rust crate를 CMake 빌드에 통합하기 위해 [Corrosion](https://github.com/corrosion-rs/corrosion)을 사용했다.

```cmake
# Tools/CMake/SEDependencies.cmake

FetchContent_Declare(Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
)
FetchContent_Declare(icu4x_src
    GIT_REPOSITORY https://github.com/unicode-org/icu4x.git
)
FetchContent_MakeAvailable(Corrosion icu4x_src)

# icu4x Rust crate를 CMake 타겟으로 가져오기
corrosion_import_crate(
    MANIFEST_PATH "${icu4x_src_SOURCE_DIR}/ffi/capi/Cargo.toml"
)
```

```cmake
# EngineCore/CMakeLists.txt

# ICU4X C++ FFI 바인딩 헤더 경로
target_include_directories(EngineCore PRIVATE
    "${icu4x_src_SOURCE_DIR}/ffi/capi/bindings/cpp"
)
```

### 4.3. 실제 사용 범위

아직까지 엔진에서 ICU4X를 사용하는 기능은 대소문자 변환뿐이다. `CaseMapper`가 Locale을 받아 UTF-8 문자열을 직접 처리한다.

```cpp
// EngineCore/Source/Core/Container/String.cpp
#include "icu4x/CaseMapper.hpp"
#include "icu4x/Locale.hpp"

// locale 문자열 파싱 (실패 시 "und" 폴백)
std::unique_ptr<icu4x::Locale> ParseLocale(const char* locale)  
{  
    if (!locale || !locale[0])  
    {  
        return icu4x::Locale::from_string("und").ok().value();  
    }  
  
    // ICU4C 형식("tr_TR") -> BCP47 형식("tr-TR")으로 변환  
    const std::string normalized = std::string_view{ locale }  
        | std::views::transform([](char c) { return c == '_' ? '-' : c; })  
        | std::ranges::to<std::string>();  
  
    auto result = icu4x::Locale::from_string(normalized);  
    SE_ASSERT(result.is_ok(), "Invalid locale string: {}", locale);  
    if (result.is_ok())  
    {  
        return std::move(result).ok().value();  
    }  
    return icu4x::Locale::from_string("und").ok().value();  
}

// 대문자 변환 예시
const std::unique_ptr<icu4x::Locale> locale_obj = ParseLocale(locale);  
auto result = icu4x::CaseMapper::uppercase_with_compiled_data(view, *locale_obj);
```

UTF-8 `string_view`를 그대로 넘기며, 결과도 UTF-8로 돌아온다.

### 4.4. 빌드 통합 시 겪은 문제들

Corrosion을 통해 Rust crate를 CMake에 통합하는 과정은 생각보다 복잡하지는 않았다.

- **Rust toolchain 요구**: 빌드 환경에 `rustup`과 stable toolchain이 반드시 설치되어 있어야 한다.
- **타겟 아키텍처 불일치**: Rust 컴파일 타겟(`x86_64-pc-windows-msvc` 등)과 CMake의 빌드 타겟이 맞지 않으면 링킹에서 실패한다. Corrosion이 CMake 타겟을 감지해 자동 설정하지만, Cross-compile 환경에서는 명시적 지정이 필요했다.
- **`crate-type` 명시 누락 (링킹 에러):** ICU4X의 `ffi/capi/Cargo.toml`에는 빌드 결과물 형식을 정의하는 `[lib]` 섹션이나 `crate-type` 명시가 빠져 있었다. Rust는 기본적으로 자신들만의 `rlib` 형식으로 빌드하기 때문에, 이를 C++ 환경에서 정상적으로 링킹하려면 CMake/Corrosion 단에서 `staticlib` 또는 `cdylib` 형식으로 빌드되도록 강제하는 추가 조치가 필요했다.
- **빌드 시간:** `FetchContent`로 icu4x 전체 소스를 내려받고 Rust로 컴파일하는 시간이 초기 빌드에 추가된다. (다만, 이후 증분 빌드에서는 영향이 없다).

---
## 5. 결과와 트레이드오프

**얻은 것:**
- UTF-8 네이티브 처리로 인코딩 변환 오버헤드 완전 제거
- 필요한 기능(현재는 CaseMapper)만 포함하는 경량 바이너리
- Unicode Consortium이 관리하는 표준 구현체 사용

**잃은 것 / 수용한 비용:**
- Rust toolchain이 빌드 환경 필수 의존성으로 추가됨
- Corrosion 기반 빌드 통합 복잡도 - 기여자 진입 장벽 상승
- ICU4C 대비 C++ API가 덜 성숙 (FFI 레이어를 통하므로 에러 메시지 등이 다소 투박함)

**미해결 / 미래 과제:**
- 현재 모든 crate를 빌드해서 사용하고 있는데, 추후 필요한 crate만 빌드해서 사용하도록 수정
- 대소문자 변환 외의 ICU4X 기능(정규화, Collation 등) 미사용 - 필요 시 확장 가능

---
## 6. 참고

- [ICU4X GitHub](https://github.com/unicode-org/icu4x)
- [Corrosion Github](https://github.com/corrosion-rs/corrosion)
- [ICU4X 테스트 레포지토리](https://github.com/gudtldn/icu4x_ffi_test)
- [CMake를 활용한 ICU4X C++ 환경 구축](https://blog.gudtldn.dev/posts/icu4x_using_from_cpp/)
