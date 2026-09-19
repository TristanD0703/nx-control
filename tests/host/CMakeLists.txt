
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

add_test(NAME nintendo_switch_controller_host_tests COMMAND nintendo_switch_controller_host_tests)

target_link_libraries(nintendo_switch_controller_host_tests PRIVATE unity m)

target_compile_options(nintendo_switch_controller_host_tests PRIVATE
  --coverage
  -O0
  -g
)

target_link_options(nintendo_switch_controller_host_tests PRIVATE
  --coverage
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

find_program(GCOVR_EXECUTABLE gcovr)

if(GCOVR_EXECUTABLE)
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
else()
  message(STATUS "gcovr not found; coverage target will not be available")
endif()
