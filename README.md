# SimpleEngine

```shell
# 1. 저장소 클론 및 폴더 진입
git clone --recurse-submodules https://github.com/gudtldn/SimpleEngine.git
cd SimpleEngine

# 2. 빌드 디렉터리 생성 및 진입
mkdir build
cd build

# 3. CMake 구성 (vcpkg 툴체인 연동)
cmake .. -DCMAKE_TOOLCHAIN_FILE="[vcpkg_root_path]/scripts/buildsystems/vcpkg.cmake"
```

## 빌드 환경

| 도구                        | 버전     | 비고                      |
|---------------------------|--------|-------------------------|
| CMake                     | 3.28+  |                         |
| vcpkg                     | 최신     | `VCPKG_ROOT` 환경변수 설정 필요 |
| Rust / rustup             | stable | ICU4X Corrosion 빌드에 필요  |
| MSVC (Visual Studio 2022) | 17.x+  |                         |
| Ninja (필수는 아님)            | 최신     | 단일 구성 생성기 사용 시 필요       |

- 좌표계: right-hand, z-up
- 1 unit = 1 meter

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

## 참고한 프로젝트 및 엔진

- [Wicked Engine](https://github.com/turanszkij/WickedEngine)
- [Unreal Engine](https://github.com/EpicGames/UnrealEngine)
- [Bevy Engine](https://github.com/bevyengine/bevy)

</details>
