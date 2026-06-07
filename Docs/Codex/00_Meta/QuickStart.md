---
작성일: 2026-05-11
최종 수정일: 2026-06-07
tags:
  - Build
  - Project Setting
작성 완료: true
---
# QuickStart - 클론부터 빌드까지

---

## 1. 필수 도구 설치

### Windows (주 개발 환경)

| 도구 | 최소 버전 | 설치 방법 | 비고 |
| --- | --- | --- | --- |
| **Git** | 2.x | `winget install Git.Git` | 서브모듈 클론에 필요 |
| **CMake** | 3.28+ | VS Installer 또는 [cmake.org](https://cmake.org/download/) | CLion 내장 CMake도 사용 가능 |
| **Visual Studio** | C++23 지원 버전 (18.x+) | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) | IDE가 아닌 **MSVC 빌드 툴체인** 목적. "C++ 데스크톱 개발" 워크로드 선택 |
| **Ninja** | 1.11+ | `winget install Ninja-build.Ninja` | CMakePresets에서 Ninja 생성기를 사용함 |
| **vcpkg** | latest | `git clone https://github.com/microsoft/vcpkg` -> `bootstrap-vcpkg.bat` | **환경변수 `VCPKG_ROOT`를 반드시 설정** |
| **Rust toolchain** | stable | [rustup.rs](https://rustup.rs/) | ICU4X(Rust FFI) 빌드에 필요 `rustup default stable` |

> [!NOTE]
> Visual Studio는 IDE로 쓰지 않아도 MSVC 컴파일러와 Windows SDK 때문에 반드시 설치해야 합니다.
> 다른 툴체인(Clang, GCC)에 대한 프리셋은 있지만, 실제 컴파일 및 테스트는 MSVC에서만 검증된 상태입니다.

---

## 2. Repository Clone

```bash
git clone --recurse-submodules https://github.com/<your-repo>/SimpleEngine.git
cd SimpleEngine
```

만약 서브모듈을 빼먹었다면:

```bash
git submodule update --init --recursive
```

서브모듈 목록 (`ThirdParty/`):

| 디렉토리 | 라이브러리 | 용도 |
| --- | --- | --- |
| `tracy` | Tracy Profiler | 프레임 프로파일링 |
| `imgui` | Dear ImGui | 에디터 UI (SDL3 + SDL_GPU 백엔드) |
| `tomlplusplus` | toml++ | 설정 파일 직렬화 |
| `cpu_features` | cpu_features | 런타임 SIMD 감지 |
| `assimp` | Assimp | 메쉬 임포트 (FBX, glTF, OBJ, Blend, MMD) |
| `picosha2` | PicoSHA2 | SHA-256 해싱 (DDC) |

---

## 3. vcpkg 의존성

`vcpkg.json` 매니페스트 모드를 사용하므로 **수동 install 불필요**. CMake configure 시 자동 설치됩니다.

설치되는 패키지:

| 패키지 | 용도 |
| --- | --- |
| `sdl3` | 윈도우 생성, 입력, GPU 추상화 |
| `sdl3-image` | 이미지 로딩 (JPEG, PNG, WebP) |
| `sdl3-shadercross` (커스텀 오버레이) | HLSL -> SPIRV 셰이더 크로스 컴파일 (Windows/Linux x64에서 DXC 사용) |
| `stduuid` | UUID 생성 (AssetId) |
| `gtest` | Google Test |
| `benchmark` | Google Benchmark |
| `efsw` | 파일 시스템 변경 감시 (FileWatcher) |

---

## 4. ICU4X (Rust FFI) — 자동 빌드

`SEDependencies.cmake`가 FetchContent로 아래를 자동 수행한다:

1. **Corrosion** (v0.6.1) — Rust ↔ CMake 브릿지
2. **icu4x** (icu@2.1.0) — Unicode 처리 라이브러리

Rust toolchain이 설치되어 있으면 **첫 configure 시 자동으로 Cargo 빌드**가 실행된다. 초회 빌드에 수 분 소요될 수 있음.

트러블슈팅:

```bash
# "cargo not found" -> Rust 경로 확인
rustc --version && cargo --version

# FetchContent 캐시 문제 -> 해당 preset의 _deps 삭제 후 재시도
# 예: debug-msvc 프리셋이라면
rm -rf Build/Debug_MSVC/_deps
```

---

## 5. Configure & Build

### 방법 A: CLion / Rider (권장)

`CMakePresets.json`을 자동으로 인식합니다.

**CLion MSVC 툴체인 설정 (최초 1회):**

1. `Settings -> Build, Execution, Deployment -> Toolchains`
2. `+` -> **Visual Studio** 선택
3. Visual Studio 설치 경로가 자동 감지되면 그대로 사용
4. `Settings -> Build, Execution, Deployment -> CMake`
5. 사용할 프리셋(예: `Debug (MSVC)`)을 프로필로 추가

이후부터는 우상단 빌드 타겟 드롭다운에서 프리셋을 선택하고 빌드하면 됩니다.

---

### 방법 B: CMake Presets (CLI)

**Windows: x64 Native Tools Command Prompt에서 실행:**

```bash
cmake --preset debug-msvc
cmake --build --preset debug-msvc
```

**Linux (GCC):**

```bash
cmake --preset debug-gcc
cmake --build --preset debug-gcc
```

**Linux (Clang):**

```bash
cmake --preset debug-clang
cmake --build --preset debug-clang
```

사용 가능한 전체 프리셋:

| 프리셋 | 컴파일러 | 빌드 타입 | Tracy | 빌드 디렉토리 |
| --- | --- | --- | --- | --- |
| `debug-msvc` | MSVC (cl) | Debug | ON | `Build/Debug_MSVC` |
| `development-msvc` | MSVC (cl) | Release + PDB | ON | `Build/Development_MSVC` |
| `release-msvc` | MSVC (cl) | Release | OFF | `Build/Release_MSVC` |
| `debug-clang-cl` | Clang-CL | Debug | ON | `Build/Debug_ClangVS` |
| `release-clang-cl` | Clang-CL | Release | OFF | `Build/Release_ClangVS` |
| `debug-gcc` | GCC | Debug | ON | `Build/Debug_GCC` |
| `development-gcc` | GCC | Release + debug | ON | `Build/Development_GCC` |
| `release-gcc` | GCC | Release | OFF | `Build/Release_GCC` |
| `debug-clang` | Clang | Debug | ON | `Build/Debug_Clang` |
| `development-clang` | Clang | Release + debug | ON | `Build/Development_Clang` |
| `release-clang` | Clang | Release | OFF | `Build/Release_Clang` |

---

### 방법 C: Visual Studio IDE

```bash
cmake -G "Visual Studio 18 2026" -A x64 -B Build
```

`Build/SimpleEngine.sln`을 열고 `EditorApp`을 시작 프로젝트로 설정.

---

## 6. 빌드 결과물 경로

```text
Binaries/
└── Windows/          (또는 Linux/)
    └── x64/
        ├── Debug/
        │   ├── EditorApp.exe   <- 에디터 실행 파일
        │   ├── EngineCore.dll  <- 엔진 코어
        │   ├── Editor.dll      <- 에디터 프레임워크
        │   ├── UnitTests.exe   <- 유닛 테스트
        │   └── Benchmarks.exe  <- 벤치마크
        ├── Development/
        └── Release/
```

---

## 7. 실행

```bash
# 에디터 실행
./Binaries/Windows/x64/Debug/EditorApp.exe

# 유닛 테스트
./Binaries/Windows/x64/Debug/UnitTests.exe

# 벤치마크
./Binaries/Windows/x64/Debug/Benchmarks.exe
```

---

## 8. 빌드 구성(Configuration) 설명

| 구성 | 최적화 | 디버그 심볼 | Assert | Tracy | 용도 |
| --- | --- | --- | --- | --- | --- |
| **Debug** | 없음 | 전체 | ON | ON | 디버깅, 단계별 실행 |
| **Development** | Release 수준 | PDB (/Zi) | ON | ON | 일상 개발 (성능 + 디버깅) |
| **Release** | 전체 | 없음 | OFF | OFF | 최종 배포 |

---

## 9. SIMD 설정 (선택)

기본값: x86에서 `AVX2`, ARM에서 `NEON`.

```bash
# SSE4.1로 낮추기 (구형 CPU 지원)
cmake --preset debug-msvc -DSE_SIMD_LEVEL=SSE4_1

# SIMD 비활성화
cmake --preset debug-msvc -DSE_SIMD_LEVEL=NONE
```

---

## 10. 자주 겪는 문제

| 증상 | 원인 | 해결 |
| --- | --- | --- |
| `VCPKG_ROOT` 관련 에러 | 환경변수 미설정 | `set VCPKG_ROOT=C:\path\to\vcpkg` (시스템 환경변수에 등록 권장) |
| `cargo` / `rustc` not found | Rust 미설치 | [rustup.rs](https://rustup.rs/) 설치 후 터미널 재시작 |
| Ninja not found | Ninja 미설치 | `winget install Ninja-build.Ninja` |
| ICU4X 빌드 실패 | FetchContent 캐시 오염 | `Build/<preset-dir>/_deps/` 삭제 후 재configure |
| 서브모듈 빌드 에러 | 서브모듈 미초기화 | `git submodule update --init --recursive` |
| LNK2019 (링크 에러) | DLL export 누락 | `SE_CORE_API` / `SE_EDITOR_API` 매크로 확인 |
| Tracy 연결 안 됨 | Tracy Profiler 미실행 | [Tracy releases](https://github.com/wolfpld/tracy/releases)에서 프로파일러 다운로드 후 실행 |
| CLion에서 MSVC 못 찾음 | 툴체인 미설정 | Settings -> Toolchains -> Visual Studio 추가 (5. 방법 A 참고) |

---

## 11. 프로젝트 구조 요약

```text
SimpleEngine/
├── EngineCore/          <- 엔진 코어 (DLL)
│   ├── Include/         <- 공개 헤더 (SimpleEngine/)
│   ├── Source/          <- 구현
│   └── Shaders/         <- 엔진 기본 셰이더
├── Editor/              <- 에디터 프레임워크 (DLL, EngineCore에 의존)
│   ├── Include/         <- 공개 헤더 (SimpleEditor/)
│   ├── Source/          <- 구현
│   └── Shaders/         <- 에디터 전용 셰이더
├── EditorApp/           <- 에디터 실행 파일 (Launcher)
├── RuntimeApp/          <- 게임 런타임 (WIP)
├── EngineTest/          <- 유닛 테스트 + 벤치마크
├── ThirdParty/          <- Git 서브모듈 (tracy, imgui, assimp, ...)
├── Tools/CMake/         <- CMake 헬퍼 모듈
├── VcpkgPorts/          <- 커스텀 vcpkg 오버레이 포트
├── Config/              <- 엔진 설정 파일
├── Docs/Codex/          <- 이 문서가 있는 곳
└── CMakePresets.json    <- 빌드 프리셋 정의
```
