
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
