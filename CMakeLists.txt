cmake_minimum_required(VERSION 3.22)
project(SimpleEngine VERSION 0.1.0)

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

# MSVC 컴파일러를 사용하는 경우에만 아래 옵션을 추가
if (MSVC)
    add_compile_options(
            "/utf-8"           # 소스 코드 파일을 UTF-8로 읽도록 지정
            "/Zc:preprocessor" # MSVC 전처리기 최신 표준 기능을 활성화
    )
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
