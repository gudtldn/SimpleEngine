cmake_minimum_required(VERSION 4.0)
project(SimpleEngine)

## 출력 디렉토리 설정
#set(OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Binaries)
#set(INTERMEDIATE_DIRECTORY ${CMAKE_SOURCE_DIR}/Intermediate)
#
## 실행 파일, 공유 라이브러리(.dll), 디버그 심볼이 생성될 위치
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_BUILD_TYPE})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_BUILD_TYPE})
#set(CMAKE_PDB_OUTPUT_DIRECTORY ${OUTPUT_DIRECTORY}/${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_BUILD_TYPE})
#
## 정적 라이브러리(.lib)가 생성될 위치
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${INTERMEDIATE_DIRECTORY}/${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}/lib)


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
