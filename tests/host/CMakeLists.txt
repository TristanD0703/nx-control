
add_executable(nintendo_switch_controller_host_tests)


target_include_directories(nintendo_switch_controller_host_tests
  PRIVATE
    ../../src
)

target_sources(nintendo_switch_controller_host_tests
  PRIVATE
    controller-state-test.c
    ../../src/controller-state/controller-state.c
    fake-timer.c
  PUBLIC
    ../../src/timer/timer.h
)

# List dependency headers here when their implementations should be mocked.
set(HOST_MOCK_HEADERS)
target_cmock_mocks(nintendo_switch_controller_host_tests ${HOST_MOCK_HEADERS})

add_test(NAME nintendo_switch_controller_host_tests COMMAND nintendo_switch_controller_host_tests)

target_link_libraries(nintendo_switch_controller_host_tests PRIVATE unity m)

target_compile_definitions(nintendo_switch_controller_host_tests
  PRIVATE
    IS_HOST_TEST=1
)

if(ENABLE_SANITIZERS)
  target_compile_options(nintendo_switch_controller_host_tests PRIVATE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -fno-sanitize-recover=all
  )

  target_link_options(nintendo_switch_controller_host_tests PRIVATE
    -fsanitize=address,undefined
  )
endif()

if(ENABLE_COVERAGE)
  target_compile_options(nintendo_switch_controller_host_tests PRIVATE
    --coverage
    -O0
    -g
  )

  target_link_options(nintendo_switch_controller_host_tests PRIVATE
    --coverage
  )

  find_program(GCOVR_EXECUTABLE gcovr REQUIRED)
  set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)

  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${GCOVR_EXECUTABLE}
      --root ${CMAKE_SOURCE_DIR}/..
      --object-directory ${CMAKE_BINARY_DIR}
      --exclude ${CMAKE_SOURCE_DIR}/
      --html-details ${COVERAGE_DIR}/index.html
      --json-summary ${COVERAGE_DIR}/summary.json
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS nintendo_switch_controller_host_tests
  )
endif()
