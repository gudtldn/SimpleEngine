---
작성일: 2026-05-13
최종 수정일: 2026-06-07
작성 완료: true
tags:
  - foundation
  - unicode
  - build
---
# 유니코드 라이브러리를 세 번 바꾼 이유 (utf8cpp -> ICU4C -> ICU4X)

> **한 줄 요약:** 단순 기능으로는 `utf8cpp`로도 충분했지만, 제대로 된 국제화(i18n)를 지원하려다 `ICU4C`의 바이너리 비대화와 UTF-16 오버헤드에 데인 후, 최종적으로 Rust 기반의 UTF-8 네이티브인 `ICU4X`에 정착했다.

**코드 진입점:**

- `ThirdParty/icu4x_bridge/Cargo.toml` - 사용하는 `icu_capi` feature 선언 (`casemap`, `compiled_data`)
- `ThirdParty/icu4x_bridge/Cargo.lock` - 버전 고정. CMake가 이 파일을 읽어 헤더 다운로드 버전을 결정함
- `Tools/CMake/SEDependencies.cmake` - Corrosion + `icu_capi` crate FetchContent 설정
- `EngineCore/Source/Core/Container/String.cpp` - ICU4X `CaseMapper` 실제 사용부

---

## 1. 왜 유니코드 라이브러리를 세 번이나 바꿨는지?

엔진의 `String` 타입은 `char` 기반 UTF-8 문자열이다. 사실 기본적인 인코딩 검증이나 단순 처리만 놓고 보면 `utf8cpp` 라이브러리로도 충분했다.

하지만 "이왕 자체 엔진을 만드는 거, 국제화(i18n)와 제대로 된 유니코드(대소문자 변환, 정규화 등)까지 완벽하게 지원해 보자"는 욕심이 생겼다. 단순한 바이트 처리로는 언어권(Locale)에 맞는 올바른 문자열 연산을 구현할 수 없었기 때문이다. 이로 인해서 프로젝트 초창기부터 현재까지 라이브러리가 여러번 바뀌는 상황이 발생했다.

---

## 2. 마이그레이션 타임라인

| 단계 | 라이브러리 | 탈락 이유 |
| --- | --- | --- |
| **1st - utf8cpp** | 경량, 헤더 온리 | 정규화(Normalization), 대소문자 변환 등 고급 기능 부재. 단순 UTF-8 인코딩 검증 수준에 그침 |
| **2nd - ICU4C** | 업계 표준, 기능 완비 | 바이너리 비대 + 내부 UTF-16 기본 사용 -> UTF-8 변환 오버헤드 |
| **-> 3rd - ICU4X (채택)** | Rust 기반 경량, UTF-8 네이티브, 기능별 선택 가능 | Corrosion 빌드 통합 복잡도 및 Rust 컴파일러 필요 (수용) |

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

### 4.2. CMake 통합 (Corrosion + 브릿지 crate)

Rust crate를 CMake 빌드에 통합하기 위해 [Corrosion](https://github.com/corrosion-rs/corrosion)을 사용했다.

초기에는 ICU4X 레포 전체를 `FetchContent`로 clone하여 `ffi/capi/Cargo.toml`을 직접 빌드했으나, ICU4X 레포 자체가 크고, 모든 모듈을 빌드하기에 초기 빌드 시간이 길었다.

**현재 구조**: `ThirdParty/icu4x_bridge/`라는 최소 Rust crate를 두고, 필요한 `icu_capi` feature만 선언했다. CMake는 이 crate의 `Cargo.lock`에서 버전을 읽어 crates.io에서 `icu_capi` crate만 직접 다운로드하여 C++ 헤더로 활용한다.

```toml
# ThirdParty/icu4x_bridge/Cargo.toml
[lib]
crate-type = ["staticlib"]

[dependencies.icu_capi]
version = "2.0"
default-features = false
features = [
    "casemap",       # CaseMapper (ToUpper / ToLower)
    "compiled_data", # 런타임 데이터 로딩 없이 바이너리에 포함
]
```

```cmake
# Tools/CMake/SEDependencies.cmake
FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(
    MANIFEST_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/icu4x_bridge/Cargo.toml"
)

# Cargo.lock 변경 시 자동 reconfigure
set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/ThirdParty/icu4x_bridge/Cargo.lock"
)

# block()으로 중간 변수 외부 누출 차단
# Cargo.lock에서 버전·checksum을 추출하여 crates.io에서 헤더 전용 다운로드
block(PROPAGATE icu_capi_src_SOURCE_DIR)
    file(READ ".../Cargo.lock" _lock)
    string(REGEX MATCH
        "name = \"icu_capi\"\nversion = \"([0-9.]+)\"\nsource = [^\n]+\nchecksum = \"([a-f0-9]+)\""
        _ "${_lock}"
    )
    FetchContent_Declare(
        icu_capi_src
        URL           "https://static.crates.io/crates/icu_capi/icu_capi-${CMAKE_MATCH_1}.crate"
        DOWNLOAD_NAME "icu_capi-${CMAKE_MATCH_1}.tar.gz"
        URL_HASH      SHA256=${CMAKE_MATCH_2}
        TLS_VERIFY    OFF  # MinGW cmake CA 인증서 부재 - URL_HASH로 무결성 보장
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(icu_capi_src)
endblock()

target_include_directories(icu4x_bridge INTERFACE
    "${icu_capi_src_SOURCE_DIR}/bindings/cpp"
)
```

`icu_capi` 버전을 올릴 때는 `Cargo.toml`만 수정하고 `cargo build`를 실행하면 `Cargo.lock`이 갱신되고, 다음 CMake configure시 새 버전의 헤더가 자동으로 다운로드된다.

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
- **`crate-type` 명시 필요**: C++ 환경에서 링킹하려면 브릿지 crate의 `Cargo.toml`에 `crate-type = ["staticlib"]`을 명시해야 한다. 누락 시 Rust 기본 형식인 `rlib`으로 빌드되어 링킹에서 실패한다.
- **`cargo:include`가 CMake로 전파되지 않음**: Corrosion은 Rust build script의 `cargo:include` 출력을 CMake의 `INTERFACE_INCLUDE_DIRECTORIES`로 자동 전파하지 않는다. 따라서 C++ 헤더 경로는 CMake 측에서 별도로 `target_include_directories`로 지정해야 한다.
- **MinGW CMake의 SSL 인증서 부재**: `FetchContent`로 crates.io에서 다운로드 시, MinGW 배포판 cmake에 CA 인증서가 없어 SSL 검증이 실패한다. `TLS_VERIFY OFF`로 우회하되, `URL_HASH SHA256=<Cargo.lock의 checksum>`으로 무결성을 보장했다.

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

- 대소문자 변환 외의 ICU4X 기능(정규화, Collation 등) 미사용 - 필요 시 `Cargo.toml`의 `features`에 추가하여 확장 가능

---

## 참고

- [ICU4X GitHub](https://github.com/unicode-org/icu4x)
- [Corrosion Github](https://github.com/corrosion-rs/corrosion)
- [ICU4X 테스트 레포지토리](https://github.com/gudtldn/icu4x_ffi_test)
- [CMake를 활용한 ICU4X C++ 환경 구축](https://blog.gudtldn.dev/posts/icu4x_using_from_cpp/)
