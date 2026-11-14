# Generate mocks for explicitly selected dependency headers and compile them into
# an existing test executable. An empty header list only links the CMock runtime.
function(target_cmock_mocks target)
    target_link_libraries(${target} PRIVATE cmock)

    if(NOT ARGN)
        return()
    endif()

    set(mock_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/${target}/mocks")
    set(mock_config "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../cmock.yml")
    set(mock_sources)
    set(mock_headers)

    foreach(header IN LISTS ARGN)
        get_filename_component(header "${header}" ABSOLUTE
            BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        get_filename_component(header_name "${header}" NAME_WE)
        get_filename_component(header_dir "${header}" DIRECTORY)
        set(mock_source "${mock_dir}/Mock${header_name}.c")
        set(mock_header "${mock_dir}/Mock${header_name}.h")

        add_custom_command(
            OUTPUT "${mock_source}" "${mock_header}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${mock_dir}"
            COMMAND ${CMAKE_COMMAND} -E env "UNITY_DIR=${unity_SOURCE_DIR}"
                "${Ruby_EXECUTABLE}" "${cmock_SOURCE_DIR}/lib/cmock.rb"
                "-o${mock_config}" "--mock_path=${mock_dir}" "${header}"
            DEPENDS
                "${header}"
                "${mock_config}"
                "${cmock_SOURCE_DIR}/lib/cmock.rb"
                "${unity_SOURCE_DIR}/auto/type_sanitizer.rb"
            COMMENT "Generating CMock mock for ${header_name}"
            VERBATIM
        )

        list(APPEND mock_sources "${mock_source}")
        list(APPEND mock_headers "${mock_header}")
        target_include_directories(${target} PRIVATE "${header_dir}")
    endforeach()

    # Test sources may include the generated headers before their .c files build.
    add_custom_target(${target}_cmock_generation
        DEPENDS ${mock_sources} ${mock_headers})
    add_dependencies(${target} ${target}_cmock_generation)
    target_sources(${target} PRIVATE ${mock_sources} ${mock_headers})
    target_include_directories(${target} PRIVATE "${mock_dir}")
endfunction()
