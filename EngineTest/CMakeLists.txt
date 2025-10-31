# Test와 Benchmark에 사용되는 라이브러리가 있는지 확인
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)


# 유닛 테스트용 실행 파일
file(GLOB_RECURSE ENGINE_UNIT_TEST_SOURCES "Source/UnitTests/*.cpp")
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

# 벤치마크 실행 파일 (GBenchmark 전용 main 사용)
file(GLOB_RECURSE ENGINE_BENCHMARK_SOURCES "Source/Benchmark/*.cpp")
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
