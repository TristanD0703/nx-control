#include <string.h>

#include "unity.h"
#include "controller-state/controller-state.h"
#include "fake-timer.h"

void setUp() {
    start_timer_spy = (FunctionSpy_t){0};
    get_timer_data_spy = (FunctionSpy_t){0};
    change_timer_duration_spy = (FunctionSpy_t){0};
    cancel_timer_spy = (FunctionSpy_t){0};
} 

void tearDown() {
    start_timer_spy = (FunctionSpy_t){0};
    get_timer_data_spy = (FunctionSpy_t){0};
    change_timer_duration_spy = (FunctionSpy_t){0};
    cancel_timer_spy = (FunctionSpy_t){0};
} 

void serializes_controller_state() {
    ControllerState_t test_controller;
    TEST_ASSERT_TRUE(init_controller(&test_controller, &fake_timer_impl));

    JoystickState_t buffer;
    serialize_controller_state(&test_controller, &buffer);
   
    int is_equal = memcmp(&buffer, &(test_controller.curr_state), sizeof(JoystickState_t));
    TEST_ASSERT_TRUE(is_equal == 0);
}

void initializes_controller() {
    ControllerState_t test_controller;
    TEST_ASSERT_TRUE(init_controller(&test_controller, &fake_timer_impl));

    TEST_ASSERT_TRUE(test_controller.timer == &fake_timer_impl);
     
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(serializes_controller_state);
  RUN_TEST(initializes_controller);
  return UNITY_END();
}
