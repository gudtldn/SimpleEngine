# Source 폴더 및 하위 폴더의 모든 .cpp 파일을 찾아 변수에 저장
file(GLOB_RECURSE ENGINE_TEST_SOURCES "Source/*.cpp")

# EngineTest를 실행 파일로 정의하고, 찾은 소스 파일들을 사용
add_executable(EngineTest ${ENGINE_TEST_SOURCES})

# 빌드가 끝난 후 Config 폴더를 EngineTest.exe가 있는 폴더로 복사
add_custom_command(TARGET EngineTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Config
        $<TARGET_FILE_DIR:EngineTest>/Config
)


# EngineTest가 EngineCore 및 다른 라이브러리들에 의존하도록 설정
target_link_libraries(EngineTest PRIVATE
        EngineCore

        # doctest
        doctest::doctest
)
