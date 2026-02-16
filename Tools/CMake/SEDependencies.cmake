# =============================================================================
# Tools/CMake/SEDependencies.cmake
# 모든 외부 패키지를 한 곳에서 찾고, 서브 프로젝트(ThirdParty) 빌드를 수행
# root CMakeLists.txt에서 include하여 사용
# =============================================================================

# vcpkg 패키지
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_shadercross REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n data)
find_package(stduuid REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(benchmark CONFIG REQUIRED)

# 로컬 서드파티 빌드
add_subdirectory(ThirdParty)
