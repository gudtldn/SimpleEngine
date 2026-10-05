# SlangSpike - Slang으로 SDL3 GPU용 셰이더를 만들어 보는 실험 도구
add_executable(SlangSpike
        Source/Main.cpp
)

target_link_libraries(SlangSpike PRIVATE
        # Project Default Options
        SE_DefaultOptions

        # Slang 컴파일러 API
        slang::slang
)

# Slang은 런타임 DLL 옆에서 표준 모듈(slang-standard-module-*)과 추가 DLL을 찾으므로 함께 복사합니다.
set(_slang_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
file(GLOB _slang_runtime_files
        "${_slang_bin_dir}/slang*.dll"
        "${_slang_bin_dir}/*.slang"
)
file(GLOB _slang_module_dirs LIST_DIRECTORIES true "${_slang_bin_dir}/slang-standard-module-*")

add_custom_command(TARGET SlangSpike POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_slang_runtime_files} $<TARGET_FILE_DIR:SlangSpike>
        COMMAND_EXPAND_LISTS
)

foreach(_module_dir IN LISTS _slang_module_dirs)
    get_filename_component(_module_name "${_module_dir}" NAME)
    if(IS_DIRECTORY "${_module_dir}")
        add_custom_command(TARGET SlangSpike POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${_module_dir}" "$<TARGET_FILE_DIR:SlangSpike>/${_module_name}"
        )
    else()
        add_custom_command(TARGET SlangSpike POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_module_dir}" $<TARGET_FILE_DIR:SlangSpike>
        )
    endif()
endforeach()

# Slang이 DXIL을 만들 때 부르는 DXC(dxcompiler.dll, 서명용 dxil.dll) 위치입니다.
target_compile_definitions(SlangSpike PRIVATE
        SLANG_SPIKE_DXC_DIR="${_slang_bin_dir}"
)
