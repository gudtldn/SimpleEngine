find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)
find_package(stduuid REQUIRED)

# 나중에 C++ 표준 라이브러리로 대체 가능하면 제거
find_package(range-v3 CONFIG REQUIRED)

add_subdirectory(tracy EXCLUDE_FROM_ALL)
set_property(TARGET TracyClient PROPERTY POSITION_INDEPENDENT_CODE ON)

# ---------------------------------------------------------------------------
# Assimp CMake Option
# https://github.com/assimp/assimp/blob/master/Build.md#cmake-build-options
# ---------------------------------------------------------------------------

# 모든 임포터 비활성화
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)

# 필요한 포맷만 명시적으로 활성화
set(ASSIMP_BUILD_OBJ_IMPORTER    TRUE CACHE BOOL "" FORCE) # .obj
set(ASSIMP_BUILD_FBX_IMPORTER    TRUE CACHE BOOL "" FORCE) # .fbx
set(ASSIMP_BUILD_GLTF_IMPORTER   TRUE CACHE BOOL "" FORCE) # .gltf, .glb
set(ASSIMP_BUILD_BLEND_IMPORTER  TRUE CACHE BOOL "" FORCE) # .blend

# 불필요한 테스트 및 유틸리티, 익스포터 비활성화
set(ASSIMP_BUILD_TESTS           OFF  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS    OFF  CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT             OFF  CACHE BOOL "" FORCE)

add_subdirectory(assimp EXCLUDE_FROM_ALL)
# ---------------------------------------------------------------------------

add_subdirectory(tomlplusplus EXCLUDE_FROM_ALL)
add_subdirectory(cpu_features EXCLUDE_FROM_ALL)

if(MSVC)
    target_compile_options(cpu_features PRIVATE
            /wd4206 # translation unit is empty
            /wd4132 # const object should be initialized
            /wd4100 # unreferenced formal parameter
    )
    target_compile_options(assimp PRIVATE
            /wd4819
            /wd4132 # const object should be initialized
    )
endif()

# SDL_shadercross
set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF)
if(NOT WIN32)
    # Windows가 아닌 플랫폼(Linux, macOS 등)에서는 DXC를 비활성화
    set(SDLSHADERCROSS_DXC OFF CACHE BOOL "Disable DXC support on non-Windows platforms")
endif()
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
