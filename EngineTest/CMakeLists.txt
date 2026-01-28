# Test와 Benchmark에 사용되는 라이브러리가 있는지 확인
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)


# 유닛 테스트용 실행 파일
file(GLOB_RECURSE ENGINE_UNIT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/UnitTests/*.cpp")

add_executable(UnitTests ${ENGINE_UNIT_TEST_SOURCES} "Source/UnitTestMain.cpp")
target_link_libraries(UnitTests PRIVATE
        EngineCore
        GTest::gtest
)
add_custom_command(TARGET UnitTests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Config
        $<TARGET_FILE_DIR:UnitTests>/Config
)

if(MSVC)
    target_compile_options(UnitTests PRIVATE
            /utf-8           # 인코딩을 UTF-8로 사용
            /Zc:preprocessor # MSVC 전처리기 최신 표준 기능을 활성화
            /Zc:__cplusplus  # 표준 __cplusplus 버전을 사용
            /W4              # 경고 수준을 최고 수준으로 설정
            /MP              # 멀티스레드 빌드 사용
            /wd4251 /wd4275 /wd4324 /wd4702

            # AVX를 명시적으로 타겟팅 | 나중에 ON, OFF로 수정
            /arch:AVX2
    )
else ()
    target_compile_options(UnitTests PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8

            -Wall -Wextra

            # AVX + FMA를 명시적으로 타겟팅 | 나중에 ON, OFF로 수정
            -mavx -mfma
    )
endif ()

# 벤치마크 실행 파일 (GBenchmark 전용 main 사용)
file(GLOB_RECURSE ENGINE_BENCHMARK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Benchmark/*.cpp")
add_executable(Benchmarks ${ENGINE_BENCHMARK_SOURCES})
target_link_libraries(Benchmarks PRIVATE
        EngineCore
        benchmark::benchmark_main
)
add_custom_command(TARGET Benchmarks POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/Config
        $<TARGET_FILE_DIR:Benchmarks>/Config
)

if(MSVC)
    target_compile_options(Benchmarks PRIVATE
            /utf-8           # 인코딩을 UTF-8로 사용
            /Zc:preprocessor # MSVC 전처리기 최신 표준 기능을 활성화
            /Zc:__cplusplus  # 표준 __cplusplus 버전을 사용
            /W4              # 경고 수준을 최고 수준으로 설정
            /MP              # 멀티스레드 빌드 사용
            /wd4251 /wd4275 /wd4324 /wd4702

            # AVX를 명시적으로 타겟팅 | 나중에 ON, OFF로 수정
            /arch:AVX2
    )
else ()
    target_compile_options(Benchmarks PRIVATE
            -finput-charset=UTF-8
            -fexec-charset=UTF-8

            -Wall -Wextra

            # AVX + FMA를 명시적으로 타겟팅 | 나중에 ON, OFF로 수정
            -mavx -mfma
    )
endif ()
