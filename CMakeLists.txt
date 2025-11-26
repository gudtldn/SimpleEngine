cmake_minimum_required(VERSION 3.22)

# 프로젝트 정의
project(SimpleEngine
        VERSION 0.1.0
        LANGUAGES C CXX
)

# --- 아키텍처 이름 설정 (x64 / x86) ---
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PROJECT_ARCHITECTURE "x64")
else ()
    set(PROJECT_ARCHITECTURE "x86")
endif ()

# 출력 디렉토리 설정
set(BINARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/Binaries/${CMAKE_SYSTEM_NAME}/${PROJECT_ARCHITECTURE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${BINARY_OUTPUT_PATH}/${CMAKE_BUILD_TYPE})

# Visual Studio 다중 구성 생성기를 위한 설정
foreach (OUTPUT_CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUT_CONFIG} OUTPUT_CONFIG_UPPER)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG}/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG})
    set(CMAKE_PDB_OUTPUT_DIRECTORY_${OUTPUT_CONFIG_UPPER} ${BINARY_OUTPUT_PATH}/${OUTPUT_CONFIG})
endforeach ()


# C++ 표준 설정
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSTIONS OFF)

# UTF-8 인코딩 사용
if (MSVC)
    add_compile_options(
            /utf-8
            /Zc:preprocessor # MSVC 전처리기 최신 표준 기능을 활성화
            /W4              # 경고 수준을 최고 수준으로 설정
            /MP              # 멀티스레드 빌드 사용
            /wd4251 /wd4275 /wd4324 /wd4702

            # AVX를 명시적으로 타겟팅
            /arch:AVX2
    )
else ()
    add_compile_options(
            -finput-charset=UTF-8
            -fexec-charset=UTF-8
            -Wall -Wextra

            # AVX + FMA를 명시적으로 타겟팅
            -mavx -mfma
    )
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
            -fexperimental-library
    )

    # Clang + UNIX (Linux/macOS) libc++를 사용하도록 설정
    if (UNIX)
        message(STATUS "Clang detected, using libc++ standard library.")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    endif ()
endif ()

# 컴파일러 빌드 설정
add_compile_definitions(
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
)

# 하위 프로젝트 추가
add_subdirectory(Editor)
add_subdirectory(EngineCore)
add_subdirectory(EngineTest)
add_subdirectory(ThirdParty)
