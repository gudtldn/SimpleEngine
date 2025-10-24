find_package(SDL3 REQUIRED)

add_subdirectory(tracy EXCLUDE_FROM_ALL)
add_subdirectory(tomlplusplus EXCLUDE_FROM_ALL)
add_subdirectory(doctest)
add_subdirectory(refl-cpp)

# SDL_shadercross
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF)
add_subdirectory(SDL_shadercross EXCLUDE_FROM_ALL)

# imgui 라이브러리 추가
add_library(imgui STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp # 데모 파일은 일반적으로 릴리즈 빌드에서 제외
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
)

target_link_libraries(imgui PRIVATE SDL3::SDL3)
