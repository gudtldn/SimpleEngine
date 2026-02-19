add_subdirectory(tracy EXCLUDE_FROM_ALL)
set_property(TARGET TracyClient PROPERTY POSITION_INDEPENDENT_CODE ON)

add_subdirectory(tomlplusplus EXCLUDE_FROM_ALL)
add_subdirectory(picosha2 EXCLUDE_FROM_ALL)

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
set(ASSIMP_NO_EXPORT             ON   CACHE BOOL "" FORCE)

# Assimp이 CMAKE_DEBUG_POSTFIX를 "d"로 전역 오염시키므로,
# 이후 서드파티 타겟에 영향을 주지 않도록 비활성화
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)

add_subdirectory(assimp EXCLUDE_FROM_ALL)
# ---------------------------------------------------------------------------

# cpu_features는 dllexport를 지원하지 않으므로 반드시 정적 빌드
set(BUILD_SHARED_LIBS_BACKUP ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(cpu_features EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_BACKUP})
unset(BUILD_SHARED_LIBS_BACKUP)

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
