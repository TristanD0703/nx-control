#include <string.h>
#include <stdlib.h>

#include "timer/timer.h"
#include "unity.h"
#include "controller-state/controller-state.h"
#include "fake-timer.h"
#include "unity_internals.h"

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
   
    int is_equal = memcmp(&buffer, &(test_controller.curr_state), sizeof(JoystickState_t)) == 0;
    TEST_ASSERT_TRUE(is_equal);
}

void initializes_controller() {
    // Guards against invalid inputs
    // Also implies that invalid inputs does not modify state
    ControllerState_t test_controller;

    TEST_ASSERT_FALSE(init_controller(NULL, NULL));
    TEST_ASSERT_FALSE(is_controller_initialized(&test_controller));
    TEST_ASSERT_FALSE(init_controller(&test_controller, NULL));
    TEST_ASSERT_FALSE(is_controller_initialized(&test_controller));
    TEST_ASSERT_FALSE(init_controller(NULL, &fake_timer_impl));
    TEST_ASSERT_FALSE(is_controller_initialized(&test_controller));

    // Properly initializes controller
    TEST_ASSERT_TRUE(init_controller(&test_controller, &fake_timer_impl));
    TEST_ASSERT_TRUE(test_controller.timer == &fake_timer_impl);
    TEST_ASSERT_TRUE(is_controller_initialized(&test_controller));
   
    JoystickState_t state;
    const JoystickState_t expected = {
      .Button = 0,
      .HAT = HAT_CENTER,
      .LX = STICK_CENTER,
      .LY = STICK_CENTER,
      .RX = STICK_CENTER,
      .RY = STICK_CENTER,
    };

    serialize_controller_state(&test_controller, &state);

    int is_equal = memcmp(&state, &expected, sizeof(JoystickState_t)) == 0;
    TEST_ASSERT_TRUE(is_equal);
}

void correctly_determines_button_presses() {
    ControllerState_t controller;
    JoystickButtonState_t check_button;

    // Controller must be initialized
    TEST_ASSERT_FALSE(is_button_pressed(&controller, NULL));

    // initialized controller means no buttons pressed
    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    check_button.op = LSTICK;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.op = RSTICK;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.op = HAT;
    check_button.value_x = HAT_BOTTOM;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_BOTTOM_LEFT;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_LEFT;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_TOP_LEFT;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_TOP;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_TOP_RIGHT;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_RIGHT;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_BOTTOM_RIGHT;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    // ...Except HAT center
    check_button.value_x = HAT_CENTER;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));
   
 
    check_button.op = BUTTON;
    check_button.value_x = SWITCH_Y;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_B;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_A;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_X;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_L;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_R;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_ZL;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_ZR;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_MINUS;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_PLUS;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_LCLICK; 
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_RCLICK;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_HOME; 
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    check_button.value_x = SWITCH_CAPTURE;
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));

    // non-centered sticks are pressed
    check_button.op = LSTICK;

    controller.curr_state.LX = STICK_MIN;
    controller.curr_state.LY = STICK_CENTER;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.LX = STICK_MAX;
    controller.curr_state.LY = STICK_CENTER;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.LX = STICK_CENTER;
    controller.curr_state.LY = STICK_MIN;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.LX = STICK_CENTER;
    controller.curr_state.LY = STICK_MAX;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.LX = STICK_MAX;
    controller.curr_state.LY = STICK_MAX;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.LX = STICK_MIN;
    controller.curr_state.LY = STICK_MIN;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    check_button.op = RSTICK;

    controller.curr_state.RX = STICK_MIN;
    controller.curr_state.RY = STICK_CENTER;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.RX = STICK_MAX;
    controller.curr_state.RY = STICK_CENTER;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.RX = STICK_CENTER;
    controller.curr_state.RY = STICK_MIN;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.RX = STICK_CENTER;
    controller.curr_state.RY = STICK_MAX;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.RX = STICK_MAX;
    controller.curr_state.RY = STICK_MAX;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    controller.curr_state.RX = STICK_MIN;
    controller.curr_state.RY = STICK_MIN;
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    // Pressed Button works
    check_button.op = BUTTON;
    check_button.value_x = SWITCH_A;
    controller.curr_state.Button = SWITCH_A; 
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    check_button.op = HAT;
    check_button.value_x = HAT_BOTTOM;
    controller.curr_state.HAT = HAT_BOTTOM; 
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &check_button));

    check_button.value_x = HAT_BOTTOM;
    controller.curr_state.HAT = HAT_TOP; 
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &check_button));
}

// Pressing each button and ensure timers are queued properly
// Ensures that timers created will always fit in stack-allocated buffer
void buttons_press_properly() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));
     
    ControllerState_t controller;
    JoystickButtonState_t button; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // We're gonna press A for 500 ms
    button.op = BUTTON;
    button.value_x = SWITCH_A;
    button.duration_ms = 500;

    // initialize spy
    void *random_info = (void *)5000;
    void *random_handle = (void *)5001;

    StartTimerArgs_t args[1] = {{ 
      .duration = button.duration_ms, 
      .cb = scheduler_callback,
      .info = random_info
    }};

    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    TimerInfo_t *info = malloc(sizeof(TimerInfo_t));
    info->controller = &controller;
    info->operation = button;

    // Simulate a callback after 500ms
    scheduler_callback((TimerMetadata_t *)info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void hat_buttons_press_properly() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));
     
    ControllerState_t controller;
    JoystickButtonState_t button; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // We're gonna press bottom d-pad for 500 ms
    button.op = HAT;
    button.value_x = HAT_BOTTOM;
    button.duration_ms = 500;

    // initialize spy
    void *random_info = (void *)5000;
    void *random_handle = (void *)5001;

    StartTimerArgs_t args[1] = {{ 
      .duration = button.duration_ms, 
      .cb = scheduler_callback,
      .info = random_info
    }};

    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    TimerInfo_t *info = malloc(sizeof(TimerInfo_t));
    info->controller = &controller;
    info->operation = button;

    // Simulate a callback after 500ms
    scheduler_callback((TimerMetadata_t *)info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void stick_buttons_press_properly() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));
     
    ControllerState_t controller;
    JoystickButtonState_t button; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // We're gonna move the stick up for 500 ms
    button.op = LSTICK;
    button.value_x = STICK_CENTER;
    button.value_y = STICK_MAX;
    button.duration_ms = 500;

    // initialize spy
    void *random_info = (void *)5000;
    void *random_handle = (void *)5001;

    StartTimerArgs_t args[1] = {{ 
      .duration = button.duration_ms, 
      .cb = scheduler_callback,
      .info = random_info
    }};

    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    TimerInfo_t *info = malloc(sizeof(TimerInfo_t));
    info->controller = &controller;
    info->operation = button;

    // Simulate a callback after 500ms
    scheduler_callback((TimerMetadata_t *)info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void mutliple_button_presses_different() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));
     
    ControllerState_t controller;
    JoystickButtonState_t button; 
    JoystickButtonState_t button2; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // We're gonna move the stick up for 1 sec and pressing A for 500 ms
    button.op = LSTICK;
    button.value_x = STICK_CENTER;
    button.value_y = STICK_MAX;
    button.duration_ms = 1000;

    button2.op = BUTTON;
    button2.value_x = SWITCH_A;
    button2.duration_ms = 500;

    // initialize spy
    void *random_info = (void *)5000;
    void *random_handle = (void *)5001;

    StartTimerArgs_t args[2] = {
        { 
            .duration = button.duration_ms, 
            .cb = scheduler_callback,
            .info = random_info
        },
        { 
            .duration = button2.duration_ms, 
            .cb = scheduler_callback,
            .info = random_info
        }
    };

    void** returns[] = { random_handle, random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    // 2 timers created
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 2);

    // Assert both buttons is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    TimerInfo_t *info = malloc(sizeof(TimerInfo_t));
    info->controller = &controller;
    info->operation = button;

    // Simulate a callback after 500ms
    scheduler_callback((TimerMetadata_t *)info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

// HAT buttons must only have one selected direction at a time 
// (pressing left then right results in right)

// Simulate a callback return and ensure button is no longer pressed
// calling release button and ensure button is nolonger pressed AND there's no timer

// Timers are created properly and API is handled correctly
// Resetting controller means all timers are cancelled

// Math is correct on sticks degree and magnitude
// All operations must fail if controller is uninitialized

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(serializes_controller_state);
  RUN_TEST(initializes_controller);
  RUN_TEST(correctly_determines_button_presses);
  RUN_TEST(buttons_press_properly);
  RUN_TEST(hat_buttons_press_properly);
  RUN_TEST(stick_buttons_press_properly);
  return UNITY_END();
}
