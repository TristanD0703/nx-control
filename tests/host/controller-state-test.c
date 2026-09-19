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

    StartTimerArgs_t args[1] = { 0 };
    StartTimerArgs_t expected_args = {
        .duration = button.duration_ms,
        .cb = scheduler_callback,
        .info = random_info,
    };

    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));

    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);
    TEST_ASSERT_TRUE(args[0].duration == button.duration_ms);
    TEST_ASSERT_TRUE(args[0].cb == scheduler_callback);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    AppTimerHandle_t cancelled_timers[1] = {0};
    cancel_timer_spy.call_args = cancelled_timers;
    scheduler_callback(args[0].info);

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

    StartTimerArgs_t args[1] = { 0 };

    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);

    TEST_ASSERT_TRUE(args[0].duration == button.duration_ms);
    TEST_ASSERT_TRUE(args[0].cb == scheduler_callback);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    AppTimerHandle_t cancelled_timers[1] = {0};
    cancel_timer_spy.call_args = cancelled_timers;
    scheduler_callback(args[0].info);
    TEST_ASSERT_TRUE(cancelled_timers[0] == random_handle);

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

    StartTimerArgs_t args[1] = { 0 };

    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);

    TEST_ASSERT_TRUE(args[0].duration == button.duration_ms);
    TEST_ASSERT_TRUE(args[0].cb == scheduler_callback);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    AppTimerHandle_t cancelled_timers[1] = {0};
    cancel_timer_spy.call_args = cancelled_timers;
    scheduler_callback(args[0].info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void multiple_button_presses_different() {
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

    StartTimerArgs_t args[2] = {0, 0};

    void** returns[] = { random_handle, random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(press_button(&controller, &button2));

    // 2 timers created
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 2);

    // Assert both buttons are pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    // Simulate a callback after 500ms
    AppTimerHandle_t cancelled_timers[2] = {0, 0};
    cancel_timer_spy.call_args = cancelled_timers;
    scheduler_callback(args[0].info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    scheduler_callback(args[1].info);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));

    TEST_ASSERT_TRUE(cancel_timer_spy.call_count == 2);
}

void multiple_button_presses_same() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // Pressing the A button twice with different timer lengths
    // First button should do as normal
    // Second button call should notice the button is already pressed, try to search for the timer,
    // change the duration to the specified by the second press

    button1.op = BUTTON;
    button1.value_x = SWITCH_A;
    button1.duration_ms = 1000;

    button2.op = BUTTON;
    button2.value_x = SWITCH_A;
    button2.duration_ms = 500;

    StartTimerArgs_t args[1] = {0};

    void *random_handle = (void *)1000;
    void** returns[] = { random_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    AppTimerHandle_t timer_data_args[1];
    TimerMetadata_t *get_timer_returns[1] = { args[0].info };
    get_timer_data_spy.call_args = timer_data_args;
    get_timer_data_spy.returns = get_timer_returns;

    ChangeTimerDurationArgs_t change_duration_args[1];
    change_timer_duration_spy.call_args = change_duration_args;

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));
    TEST_ASSERT_TRUE(change_timer_duration_spy.call_count == 1);
    TEST_ASSERT_TRUE(change_duration_args[0].duration_ms == button2.duration_ms);
    TEST_ASSERT_TRUE(change_duration_args[0].handle == random_handle);
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 1);

    AppTimerHandle_t canceled_handles[1] = { 0 }; 
    cancel_timer_spy.call_args  = canceled_handles;
    scheduler_callback(args[0].info);

    TEST_ASSERT_TRUE(cancel_timer_spy.call_count == 1);
    TEST_ASSERT_TRUE(canceled_handles[0] == random_handle);
}

void multiple_button_presses_hat() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // HAT buttons must only have one selected direction at a time 
    // (pressing left then right results in right)

    // should modify the timer in place
    button1.op = HAT;
    button1.value_x = HAT_LEFT;
    button1.duration_ms = 500;

    button2.op = HAT;
    button2.value_x = HAT_RIGHT;
    button2.duration_ms = 600;

    StartTimerArgs_t args[1] = { 0 };

    void *left_timer_handle = (void *)1000;
    void** returns[] = { left_timer_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    AppTimerHandle_t timer_data_args[1];
    TimerMetadata_t *get_timer_returns[1] = { args[0].info };
    get_timer_data_spy.call_args = timer_data_args;
    get_timer_data_spy.returns = get_timer_returns;

    ChangeTimerDurationArgs_t change_duration_args[1];
    change_timer_duration_spy.call_args = change_duration_args;

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(change_duration_args[0].handle == left_timer_handle);
    TEST_ASSERT_TRUE(change_duration_args[0].duration_ms == button2.duration_ms);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button1));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    AppTimerHandle_t canceled_handles[1] = { 0 }; 
    cancel_timer_spy.call_args = canceled_handles;
    scheduler_callback(args[0].info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
    TEST_ASSERT_TRUE(cancel_timer_spy.call_count == 1);
    TEST_ASSERT_TRUE(canceled_handles[0] == left_timer_handle);
}

// back-to-back stick updates modify the same timer instead of
// creating a new one

void multiple_button_presses_l_stick() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // stick buttons must only have one selected direction at a time 
    // (pressing left then right results in right)

    // should modify the timer in place
    button1.op = LSTICK;
    button1.value_x = STICK_MIN;
    button1.value_x = STICK_CENTER;
    button1.duration_ms = 500;

    button2.op = LSTICK;
    button2.value_x = STICK_MAX;
    button1.value_x = STICK_CENTER;
    button2.duration_ms = 600;

    StartTimerArgs_t args[1] = { 0 };

    void *left_timer_handle = (void *)1000;
    void** returns[] = { left_timer_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    AppTimerHandle_t timer_data_args[1];
    TimerMetadata_t *get_timer_returns[1] = { args[0].info };
    get_timer_data_spy.call_args = timer_data_args;
    get_timer_data_spy.returns = get_timer_returns;

    ChangeTimerDurationArgs_t change_duration_args[1];
    change_timer_duration_spy.call_args = change_duration_args;

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(change_duration_args[0].handle == left_timer_handle);
    TEST_ASSERT_TRUE(change_duration_args[0].duration_ms == button2.duration_ms);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    AppTimerHandle_t canceled_handles[1] = { 0 }; 
    cancel_timer_spy.call_args = canceled_handles;
    scheduler_callback(args[0].info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
    TEST_ASSERT_TRUE(cancel_timer_spy.call_count == 1);
    TEST_ASSERT_TRUE(canceled_handles[0] == left_timer_handle);
}


void multiple_button_presses_r_stick() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // stick buttons must only have one selected direction at a time 
    // (pressing left then right results in right)

    // should modify the timer in place
    button1.op = RSTICK;
    button1.value_x = STICK_MIN;
    button1.value_x = STICK_CENTER;
    button1.duration_ms = 500;

    button2.op = RSTICK;
    button2.value_x = STICK_MAX;
    button1.value_x = STICK_CENTER;
    button2.duration_ms = 600;

    StartTimerArgs_t args[1] = { 0 };

    void *left_timer_handle = (void *)1000;
    void** returns[] = { left_timer_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    AppTimerHandle_t timer_data_args[1];
    TimerMetadata_t *get_timer_returns[1] = { args[0].info };
    get_timer_data_spy.call_args = timer_data_args;
    get_timer_data_spy.returns = get_timer_returns;

    ChangeTimerDurationArgs_t change_duration_args[1];
    change_timer_duration_spy.call_args = change_duration_args;

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(change_duration_args[0].handle == left_timer_handle);
    TEST_ASSERT_TRUE(change_duration_args[0].duration_ms == button2.duration_ms);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    AppTimerHandle_t canceled_handles[1] = { 0 }; 
    cancel_timer_spy.call_args = canceled_handles;
    scheduler_callback(args[0].info);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
    TEST_ASSERT_TRUE(cancel_timer_spy.call_count == 1);
    TEST_ASSERT_TRUE(canceled_handles[0] == left_timer_handle);
}

void multiple_button_presses_same_one_different_interleave() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;
    JoystickButtonState_t button3;

    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    // Pressing the A button twice with different timer lengths
    // First button should do as normal
    // Second button call should notice the button is already pressed, try to search for the timer,
    // change the duration to the specified by the second press

    button1.op = BUTTON;
    button1.value_x = SWITCH_A;
    button1.duration_ms = 1000;

    button2.op = BUTTON;
    button2.value_x = SWITCH_B;
    button2.duration_ms = 1000;

    button3.op = BUTTON;
    button3.value_x = SWITCH_A;
    button3.duration_ms = 500;

    StartTimerArgs_t args[2] = {0, 0};

    void *a_handle = (void *)1000;
    void *b_handle = (void *)1001;
    void** returns[] = { a_handle, b_handle };

    start_timer_spy.call_args = args;
    start_timer_spy.returns = returns;

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    AppTimerHandle_t timer_data_args[1];
    TimerMetadata_t *get_timer_returns[1] = { args[0].info };
    get_timer_data_spy.call_args = timer_data_args;
    get_timer_data_spy.returns = get_timer_returns;

    ChangeTimerDurationArgs_t change_duration_args[1];
    change_timer_duration_spy.call_args = change_duration_args;

    press_button(&controller, &button2);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    press_button(&controller, &button3);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));
    TEST_ASSERT_TRUE(change_timer_duration_spy.call_count == 1);
    TEST_ASSERT_TRUE(change_duration_args[0].duration_ms == button3.duration_ms);
    TEST_ASSERT_TRUE(change_duration_args[0].handle == a_handle);
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 2);

    AppTimerHandle_t canceled_handles[2] = { 0, 0 }; 
    cancel_timer_spy.call_args  = canceled_handles;
    scheduler_callback(args[0].info);
    scheduler_callback(args[1].info);

    TEST_ASSERT_TRUE(cancel_timer_spy.call_count == 2);
    TEST_ASSERT_TRUE(canceled_handles[0] == a_handle);
    TEST_ASSERT_TRUE(canceled_handles[1] == b_handle);
}

void can_press_all_buttons() {
    StartTimerArgs_t start_timer_args[17] = {0};
    void *start_timer_returns[17] = {0};

    start_timer_spy.call_args = start_timer_args;
    start_timer_spy.returns = start_timer_returns;

    JoystickButtonState_t buttons[17] = {
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_Y 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_LCLICK 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_CAPTURE 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_HOME 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_RCLICK 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_PLUS 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_MINUS 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_ZR 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_ZL 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_R 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_L 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_X 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_A 
        },
        {
          .op = BUTTON,
          .duration_ms = 500,
          .value_x = SWITCH_B 
        },
        {
          .op = HAT,
          .duration_ms = 500,
          .value_x = HAT_TOP_LEFT 
        },
        {
          .op = LSTICK,
          .duration_ms = 500,
          .value_x = STICK_MIN,
          .value_y = STICK_MAX
        },
        {
          .op = LSTICK,
          .duration_ms = 500,
          .value_x = STICK_MAX,
          .value_y = STICK_MIN
        },
    };

    ControllerState_t controller;
    TEST_ASSERT_TRUE(init_controller(&controller, &fake_timer_impl));

    TEST_ASSERT_TRUE(press_button_combo(&controller, buttons, 17));
    TEST_ASSERT_TRUE(start_timer_spy.call_count == 17);

    for (int i = 0; i < 17; i++) {
        TEST_ASSERT_TRUE(is_button_pressed(&controller, &buttons[i]));
    }

    for (int i = 0; i < 17; i++) {
        scheduler_callback(start_timer_args[i].info);
    }

    for (int i = 0; i < 17; i++) {
        TEST_ASSERT_FALSE(is_button_pressed(&controller, &buttons[i]));
    }
}

// "raw" updates to the controller does cancels all timers
// if done externally

// Timers are created properly and API is handled correctly
// Resetting controller means all timers are cancelled

// Math is correct on sticks degree and magnitude


int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(serializes_controller_state);
    RUN_TEST(initializes_controller);
    RUN_TEST(correctly_determines_button_presses);
    RUN_TEST(buttons_press_properly);
    RUN_TEST(hat_buttons_press_properly);
    RUN_TEST(stick_buttons_press_properly);
    RUN_TEST(multiple_button_presses_different);
    RUN_TEST(multiple_button_presses_same);
    RUN_TEST(multiple_button_presses_hat);
    RUN_TEST(multiple_button_presses_l_stick);
    RUN_TEST(multiple_button_presses_r_stick);
    RUN_TEST(multiple_button_presses_same_one_different_interleave);
    RUN_TEST(can_press_all_buttons);
    return UNITY_END();
}
