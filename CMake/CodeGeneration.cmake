#[[
이 모듈은 C++ 리플렉션 코드를 자동으로 생성하는 함수를 제공합니다.
지정된 헤더 디렉토리를 스캔하고, 리플렉션 매크로가 포함된 파일에 대해 .gen.cpp 파일을 생성한 후, 이를 지정된 타겟에 포함시킵니다.
]]

# --- venv 관련 공통 변수를 파일 상단에 정의 ---
set(VENV_DIR "${CMAKE_SOURCE_DIR}/.venv")
if (WIN32)
    set(VENV_PYTHON_EXECUTABLE "${VENV_DIR}/Scripts/python.exe")
else ()
    set(VENV_PYTHON_EXECUTABLE "${VENV_DIR}/bin/python")
endif ()

#
# 지정된 타겟에 대한 리플렉션 코드 생성 파이프라인을 설정합니다.
#
# USAGE:
# setup_reflection_generation(
#     TARGET                <target_name>        # 코드를 추가할 라이브러리/실행 파일 타겟
#     SCAN_DIRECTORIES      <dir1> [<dir2> ...]  # 스캔할 헤더 디렉토리(들)
#     GENERATED_SOURCES_VAR <var_name>           # 생성된 파일 목록을 저장할 변수 이름
# )
#
function(setup_reflection_generation)
    # 함수 인자 파싱
    set(options "")
    set(oneValueArgs TARGET GENERATED_SOURCES_VAR)
    set(multiValueArgs SCAN_DIRECTORIES)
    cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # venv 존재 여부를 확인
    if (NOT EXISTS "${VENV_PYTHON_EXECUTABLE}")
        message(FATAL_ERROR "Python virtual environment not found at ${VENV_DIR}. "
                "Please run setup_venv.bat (Windows) or setup_venv.sh (Linux/macOS) first.")
    endif ()

    # 생성된 파일 목록을 담을 .cmake 파일의 경로를 정의
    set(GENERATED_SOURCES_CMAKE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${ARG_TARGET}_generated_sources.cmake")
    # 생성된 .cpp 파일들이 위치할 디렉토리 경로
    set(GENERATED_SOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/${ARG_TARGET}_generated")
    # .cmake 파일이 사용할 CMake 변수 이름
    set(GENERATED_SOURCES_VAR_NAME "${ARG_TARGET}_GENERATED_SOURCES")

    # 스크립트를 실행하는 커스텀 타겟을 정의
    add_custom_target(${ARG_TARGET}_CodeGenerator ALL
            COMMAND ${VENV_PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Scripts/generate_reflection.py
            --scan-dirs ${ARG_SCAN_DIRECTORIES}
            --output-dir "${GENERATED_SOURCES_DIR}"
            --output-cmake-file "${GENERATED_SOURCES_CMAKE_FILE}"
            --cmake-var-name "${GENERATED_SOURCES_VAR_NAME}"

            # 스캔 디렉토리의 CMakeLists.txt에 의존하여 파일 추가/삭제를 감지
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
            ${CMAKE_SOURCE_DIR}/Scripts/generate_reflection.py
            COMMENT "Running Python Header Tool for ${ARG_TARGET}"
            USES_TERMINAL
    )

    # 메인 타겟이 코드 생성 타겟에 의존
    add_dependencies(${ARG_TARGET} ${ARG_TARGET}_CodeGenerator)

    # 생성된 .cmake 파일을 include함
    include(${GENERATED_SOURCES_CMAKE_FILE} OPTIONAL)

    # 생성된 소스 파일 목록을 PARENT_SCOPE 변수에 전달
    #    호출부에서 지정한 변수 이름(ARG_GENERATED_SOURCES_VAR)을 사용
    set(${ARG_GENERATED_SOURCES_VAR} ${${GENERATED_SOURCES_VAR_NAME}} PARENT_SCOPE)
endfunction()
