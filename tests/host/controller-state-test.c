#include <string.h>

#include "unity.h"
#include "controller-state/controller-state.h"
#include "Mocktimer.h"

typedef struct {
    SchedulerCallback_t callback;
    TimerMetadata_t *metadata;
} CapturedTimer_t;

static CapturedTimer_t captured_timers[MAX_TIMER_COUNT];
static uint32_t controller_update_count;
static JoystickState_t last_controller_update;

static void capture_controller_update(JoystickState_t *state) {
    controller_update_count++;
    last_controller_update = *state;
}

static AppTimerHandle_t capture_timer(
    uint32_t duration,
    SchedulerCallback_t callback,
    TimerMetadata_t *metadata,
    int call_index
) {
    (void)duration; // CMock checks the expected duration before this callback.
    TEST_ASSERT_LESS_THAN(MAX_TIMER_COUNT, call_index);
    TEST_ASSERT_NOT_NULL(metadata);
    captured_timers[call_index] = (CapturedTimer_t){
        .callback = callback,
        .metadata = metadata,
    };
    return &captured_timers[call_index];
}

static void expect_timer_start(uint32_t duration, uint8_t index) {
    create_and_start_timer_ExpectAndReturn(
        duration, scheduler_callback, NULL, &captured_timers[index]
    );
    // The controller allocates metadata internally; capture its actual address.
    create_and_start_timer_IgnoreArg_info();
}

static void expire_timer(uint8_t index) {
    CapturedTimer_t timer = captured_timers[index];
    TEST_ASSERT_NOT_NULL(timer.callback);
    TEST_ASSERT_NOT_NULL(timer.metadata);
    captured_timers[index].metadata = NULL;
    timer.callback(timer.metadata);
}

void setUp(void) {
    Mocktimer_Init();
    memset(captured_timers, 0, sizeof(captured_timers));
    controller_update_count = 0;
    memset(&last_controller_update, 0, sizeof(last_controller_update));
    create_and_start_timer_AddCallback(capture_timer);
}

void tearDown(void) {
    if (TEST_PROTECT()) {
        Mocktimer_Verify();
    }
    Mocktimer_Destroy();
}

void serializes_controller_state() {
    ControllerState_t test_controller;
    TEST_ASSERT_TRUE(init_controller(&test_controller));

    JoystickState_t buffer;
    serialize_controller_state(&test_controller, &buffer);
   
    int is_equal = memcmp(&buffer, &(test_controller.curr_state), sizeof(JoystickState_t)) == 0;
    TEST_ASSERT_TRUE(is_equal);
}

void initializes_controller() {
    // Guards against invalid inputs
    ControllerState_t test_controller = {0};

    TEST_ASSERT_FALSE(init_controller(NULL));
    TEST_ASSERT_FALSE(is_controller_initialized(&test_controller));

    // Properly initializes controller
    TEST_ASSERT_TRUE(init_controller(&test_controller));
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

void initialization_discards_previous_callback() {
    ControllerState_t controller;
    memset(&controller, 0xa5, sizeof(controller));
    TEST_ASSERT_TRUE(init_controller(&controller));
    set_controller_update_cb(&controller, capture_controller_update);

    TEST_ASSERT_TRUE(init_controller(&controller));

    TEST_ASSERT_TRUE(is_controller_initialized(&controller));
    TEST_ASSERT_TRUE(controller.on_update == NULL);
    TEST_ASSERT_EQUAL_UINT32(0, controller_update_count);
}

void correctly_determines_button_presses() {
    ControllerState_t controller = {0};
    JoystickButtonState_t check_button;

    // Controller must be initialized
    TEST_ASSERT_FALSE(is_button_pressed(&controller, NULL));

    // initialized controller means no buttons pressed
    TEST_ASSERT_TRUE(init_controller(&controller));

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

    ControllerState_t controller = {0};
    JoystickButtonState_t button; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller));

    // We're gonna press A for 500 ms
    button.op = BUTTON;
    button.value_x = SWITCH_A;
    button.duration_ms = 500;

    expect_timer_start(button.duration_ms, 0);

    TEST_ASSERT_TRUE(press_button(&controller, &button));

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void failed_timer_creation_does_not_leak_metadata() {
    ControllerState_t controller;
    JoystickButtonState_t button = {
        .op = BUTTON,
        .value_x = SWITCH_A,
        .duration_ms = 500,
    };
    TEST_ASSERT_TRUE(init_controller(&controller));

    // Use CMock's queued return values instead of capturing successful timers.
    create_and_start_timer_AddCallback(NULL);

    for (uint8_t attempt = 0; attempt < MAX_TIMER_COUNT; attempt++) {
        create_and_start_timer_ExpectAndReturn(
            button.duration_ms, scheduler_callback, NULL, NULL
        );
        create_and_start_timer_IgnoreArg_info();
        TEST_ASSERT_TRUE(press_button(&controller, &button));
        TEST_ASSERT_NULL(controller.timer_handles[0]);

        release_button(&controller, &button);
    }
}

void hat_buttons_press_properly() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));

    ControllerState_t controller = {0};
    JoystickButtonState_t button; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller));

    // We're gonna press bottom d-pad for 500 ms
    button.op = HAT;
    button.value_x = HAT_BOTTOM;
    button.duration_ms = 500;

    expect_timer_start(button.duration_ms, 0);

    TEST_ASSERT_TRUE(press_button(&controller, &button));

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void stick_buttons_press_properly() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));

    ControllerState_t controller = {0};
    JoystickButtonState_t button; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller));

    // We're gonna move the stick up for 500 ms
    button.op = LSTICK;
    button.value_x = STICK_CENTER;
    button.value_y = STICK_MAX;
    button.duration_ms = 500;

    expect_timer_start(button.duration_ms, 0);

    TEST_ASSERT_TRUE(press_button(&controller, &button));

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void multiple_button_presses_different() {
    TEST_ASSERT_FALSE(press_button(NULL, NULL));

    ControllerState_t controller = {0};
    JoystickButtonState_t button; 
    JoystickButtonState_t button2; 
    TEST_ASSERT_FALSE(press_button(&controller, NULL));
    TEST_ASSERT_FALSE(press_button(NULL, &button));

    TEST_ASSERT_TRUE(init_controller(&controller));

    // We're gonna move the stick up for 1 sec and pressing A for 500 ms
    button.op = LSTICK;
    button.value_x = STICK_CENTER;
    button.value_y = STICK_MAX;
    button.duration_ms = 1000;

    button2.op = BUTTON;
    button2.value_x = SWITCH_A;
    button2.duration_ms = 500;

    expect_timer_start(button.duration_ms, 0);
    expect_timer_start(button2.duration_ms, 1);

    TEST_ASSERT_TRUE(press_button(&controller, &button));
    TEST_ASSERT_TRUE(press_button(&controller, &button2));

    // Assert both buttons are pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    // Simulate a callback after 500ms
    cancel_timer_Expect(&captured_timers[1]);
    expire_timer(1);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));

    // The stick's timer expires after 1000ms.
    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
}

void multiple_button_presses_same() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller));

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

    expect_timer_start(button1.duration_ms, 0);

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    change_timer_duration_Expect(&captured_timers[0], button2.duration_ms);

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
}

void multiple_button_presses_hat() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller));

    // HAT buttons must only have one selected direction at a time 
    // (pressing left then right results in right)

    // should modify the timer in place
    button1.op = HAT;
    button1.value_x = HAT_LEFT;
    button1.duration_ms = 500;

    button2.op = HAT;
    button2.value_x = HAT_RIGHT;
    button2.duration_ms = 600;

    expect_timer_start(button1.duration_ms, 0);

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    change_timer_duration_Expect(&captured_timers[0], button2.duration_ms);

    press_button(&controller, &button2);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button1));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
}

// back-to-back stick updates modify the same timer instead of
// creating a new one

void multiple_button_presses_l_stick() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller));

    // stick buttons must only have one selected direction at a time 
    // (pressing left then right results in right)

    // should modify the timer in place
    button1.op = LSTICK;
    button1.value_x = STICK_MIN;
    button1.value_y = STICK_CENTER;
    button1.duration_ms = 500;

    button2.op = LSTICK;
    button2.value_x = STICK_MAX;
    button2.value_y = STICK_CENTER;
    button2.duration_ms = 600;

    expect_timer_start(button1.duration_ms, 0);

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    change_timer_duration_Expect(&captured_timers[0], button2.duration_ms);

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
}


void multiple_button_presses_r_stick() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;

    TEST_ASSERT_TRUE(init_controller(&controller));

    // stick buttons must only have one selected direction at a time 
    // (pressing left then right results in right)

    // should modify the timer in place
    button1.op = RSTICK;
    button1.value_x = STICK_MIN;
    button1.value_y = STICK_CENTER;
    button1.duration_ms = 500;

    button2.op = RSTICK;
    button2.value_x = STICK_MAX;
    button2.value_y = STICK_CENTER;
    button2.duration_ms = 600;

    expect_timer_start(button1.duration_ms, 0);

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    change_timer_duration_Expect(&captured_timers[0], button2.duration_ms);

    press_button(&controller, &button2);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
}

void multiple_button_presses_same_one_different_interleave() {
    ControllerState_t controller;
    JoystickButtonState_t button1;
    JoystickButtonState_t button2;
    JoystickButtonState_t button3;

    TEST_ASSERT_TRUE(init_controller(&controller));

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

    expect_timer_start(button1.duration_ms, 0);
    expect_timer_start(button2.duration_ms, 1);

    press_button(&controller, &button1);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    press_button(&controller, &button2);
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    change_timer_duration_Expect(&captured_timers[0], button3.duration_ms);
    press_button(&controller, &button3);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button1));

    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button1));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button2));

    cancel_timer_Expect(&captured_timers[1]);
    expire_timer(1);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button2));
}

void can_press_all_buttons() {
    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        expect_timer_start(500, i);
    }

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
          .op = RSTICK,
          .duration_ms = 500,
          .value_x = STICK_MAX,
          .value_y = STICK_MIN
        },
    };

    ControllerState_t controller;
    TEST_ASSERT_TRUE(init_controller(&controller));

    TEST_ASSERT_TRUE(press_button(&controller, &buttons[14]));

    TEST_ASSERT_TRUE(press_button(&controller, &buttons[15]));
    TEST_ASSERT_TRUE(press_button(&controller, &buttons[16]));

    TEST_ASSERT_TRUE(press_button_combo(&controller, buttons, 14));

    for (int i = 0; i < 17; i++) {
        TEST_ASSERT_TRUE(is_button_pressed(&controller, &buttons[i]));
    }

    for (int i = 0; i < 17; i++) {
        cancel_timer_Expect(&captured_timers[i]);
        expire_timer(i);
    }

    for (int i = 0; i < 17; i++) {
        TEST_ASSERT_FALSE(is_button_pressed(&controller, &buttons[i]));
    }
}

// "raw" updates to the controller does cancels all timers
// if done externally
void raw_updates_cancel_timers() {
    ControllerState_t controller;
    JoystickButtonState_t stick1;
    JoystickButtonState_t stick2;
    TEST_ASSERT_TRUE(init_controller(&controller));

    stick1.op = LSTICK;
    stick1.duration_ms = 500;
    stick1.value_x = STICK_MIN;
    stick1.value_y = STICK_MAX;

    expect_timer_start(stick1.duration_ms, 0);

    TEST_ASSERT_TRUE(press_button(&controller, &stick1));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &stick1));

    stick2.op = LSTICK;
    stick2.value_x = 100;
    stick2.value_y = 50;

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    cancel_timer_Expect(&captured_timers[0]);

    update_sticks_raw(&controller, 100, 50, LSTICK);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &stick2));
    TEST_ASSERT_TRUE(controller.curr_state.LX == 100);
    TEST_ASSERT_TRUE(controller.curr_state.LY == 50);
}

void raw_stick_updates_cancel_only_stick() {
    ControllerState_t controller;
    JoystickButtonState_t stick1;
    JoystickButtonState_t stick2;
    TEST_ASSERT_TRUE(init_controller(&controller));

    stick1.op = BUTTON;
    stick1.duration_ms = 500;
    stick1.value_x = SWITCH_L;

    expect_timer_start(stick1.duration_ms, 0);

    TEST_ASSERT_TRUE(press_button(&controller, &stick1));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &stick1));

    stick2.op = RSTICK;
    stick2.value_x = 100;
    stick2.value_y = 50;

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );

    update_sticks_raw(&controller, 100, 50, RSTICK);

    TEST_ASSERT_TRUE(is_button_pressed(&controller, &stick2));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &stick1));

    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &stick1));
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &stick2));
}
// Timers are created properly and API is handled correctly
// Resetting controller means all timers are cancelled

// Math is correct on sticks degree and magnitude
void deg_mag_updates_properly() {
    ControllerState_t controller;
    JoystickButtonState_t stick1;
    TEST_ASSERT_TRUE(init_controller(&controller));

    stick1.op = RSTICK;
    stick1.duration_ms = 500;

    // Halfway right
    update_sticks_deg_mag_raw(&controller, 0, .5, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RX == 192);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_CENTER);

    // Halfway up 
    update_sticks_deg_mag_raw(&controller, 90, .5, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == 192);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);

    // Halfway 30 degrees
    update_sticks_deg_mag_raw(&controller, 30, .5, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == 160);
    TEST_ASSERT_TRUE(controller.curr_state.RX == 183);

    // Full 30 degrees
    update_sticks_deg_mag_raw(&controller, 30, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == 192);
    TEST_ASSERT_TRUE(controller.curr_state.RX == 239);

    // Completely center
    update_sticks_deg_mag_raw(&controller, 360, 0, RSTICK);
    TEST_ASSERT_FALSE(is_button_pressed(&controller, &stick1));

    // up
    update_sticks_deg_mag_raw(&controller, 90, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MAX);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);

    // Large magnitude clamps to max
    update_sticks_deg_mag_raw(&controller, 90, 2, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MAX);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);

    // left
    update_sticks_deg_mag_raw(&controller, 180, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_CENTER);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_MIN);

    // down 
    update_sticks_deg_mag_raw(&controller, 270, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MIN);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);

    // right 
    update_sticks_deg_mag_raw(&controller, 360, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_CENTER);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_MAX);

    update_sticks_deg_mag_raw(&controller, 0, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_CENTER);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_MAX);

    // Congruent angles work too
    update_sticks_deg_mag_raw(&controller, 450, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MAX);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);

    update_sticks_deg_mag_raw(&controller, -450, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MIN);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);

    update_sticks_deg_mag_raw(&controller, -90, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MIN);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_CENTER);


    // negative magnitude
    update_sticks_deg_mag_raw(&controller, 360, -1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_CENTER);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_MIN);

    update_sticks_deg_mag_raw(&controller, 30, -.5, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RY == 96);
    TEST_ASSERT_TRUE(controller.curr_state.RX == 73);
}

void full_magnitude_near_positive_axes_does_not_overflow() {
    ControllerState_t controller;
    TEST_ASSERT_TRUE(init_controller(&controller));

    update_sticks_deg_mag_raw(&controller, 1, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_MAX);
    TEST_ASSERT_TRUE(controller.curr_state.RY == 130);

    update_sticks_deg_mag_raw(&controller, 89, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RX == 130);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MAX);

    update_sticks_deg_mag_raw(&controller, 91, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RX == 126);
    TEST_ASSERT_TRUE(controller.curr_state.RY == STICK_MAX);

    update_sticks_deg_mag_raw(&controller, 359, 1, RSTICK);
    TEST_ASSERT_TRUE(controller.curr_state.RX == STICK_MAX);
    TEST_ASSERT_TRUE(controller.curr_state.RY == 126);
}

void deg_mag_stick_press_properly() {
    ControllerState_t controller;
    JoystickButtonState_t button; 

    TEST_ASSERT_TRUE(init_controller(&controller));

    // We're gonna press up for 500 ms
    button.op = LSTICK;
    button.duration_ms = 500;

    expect_timer_start(button.duration_ms, 0);

    update_sticks_deg_mag(&controller, 90, 1, LSTICK, 500);

    // Assert button is pressed. Must be consistent with other functions
    TEST_ASSERT_TRUE(is_button_pressed(&controller, &button));

    // Simulate a callback after 500ms
    cancel_timer_Expect(&captured_timers[0]);
    expire_timer(0);

    TEST_ASSERT_FALSE(is_button_pressed(&controller, &button));
}

void resets_controller() {
    JoystickButtonState_t buttons[3] = {
        { .op = BUTTON, .duration_ms = 500, .value_x = SWITCH_A },
        { .op = BUTTON, .duration_ms = 500, .value_x = SWITCH_B },
        { .op = BUTTON, .duration_ms = 500, .value_x = SWITCH_X }
    };
    ControllerState_t controller;
    TEST_ASSERT_TRUE(init_controller(&controller));

    for (uint8_t i = 0; i < 3; i++) {
        expect_timer_start(buttons[i].duration_ms, i);
    }

    TEST_ASSERT_TRUE(press_button_combo(&controller, buttons, 3));
 
    for (uint8_t i = 0; i < 3; i++) {
        get_timer_metadata_ExpectAndReturn(
            &captured_timers[i], captured_timers[i].metadata
        );
        cancel_timer_Expect(&captured_timers[i]);
    }

    reset_controller(&controller);
    TEST_ASSERT_EQUAL_UINT16(0, controller.curr_state.Button);
    TEST_ASSERT_TRUE(is_controller_initialized(&controller));
    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        TEST_ASSERT_NULL(controller.timer_handles[i]);
    }
}

void reset_preserves_callback_and_notifies() {
    ControllerState_t controller;
    TEST_ASSERT_TRUE(init_controller(&controller));
    JoystickButtonState_t button = {
        .op = BUTTON,
        .value_x = SWITCH_A,
        .duration_ms = 500,
    };
    expect_timer_start(button.duration_ms, 0);
    TEST_ASSERT_TRUE(press_button(&controller, &button));
    set_controller_update_cb(&controller, capture_controller_update);

    get_timer_metadata_ExpectAndReturn(
        &captured_timers[0], captured_timers[0].metadata
    );
    cancel_timer_Expect(&captured_timers[0]);
    reset_controller(&controller);

    TEST_ASSERT_TRUE(is_controller_initialized(&controller));
    TEST_ASSERT_TRUE(controller.on_update == capture_controller_update);
    TEST_ASSERT_EQUAL_UINT32(1, controller_update_count);
    TEST_ASSERT_EQUAL_UINT16(0, last_controller_update.Button);
    TEST_ASSERT_EQUAL_UINT8(HAT_CENTER, last_controller_update.HAT);
    TEST_ASSERT_EQUAL_UINT8(STICK_CENTER, last_controller_update.LX);
    TEST_ASSERT_EQUAL_UINT8(STICK_CENTER, last_controller_update.LY);
    TEST_ASSERT_EQUAL_UINT8(STICK_CENTER, last_controller_update.RX);
    TEST_ASSERT_EQUAL_UINT8(STICK_CENTER, last_controller_update.RY);
    TEST_ASSERT_EQUAL_MEMORY(
        &controller.curr_state, &last_controller_update, sizeof(JoystickState_t)
    );

    update_sticks_raw(&controller, STICK_MIN, STICK_MAX, LSTICK);
    TEST_ASSERT_EQUAL_UINT32(2, controller_update_count);
    TEST_ASSERT_EQUAL_UINT8(STICK_MIN, last_controller_update.LX);
    TEST_ASSERT_EQUAL_UINT8(STICK_MAX, last_controller_update.LY);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(serializes_controller_state);
    RUN_TEST(initializes_controller);
    RUN_TEST(initialization_discards_previous_callback);
    RUN_TEST(correctly_determines_button_presses);
    RUN_TEST(buttons_press_properly);
    RUN_TEST(failed_timer_creation_does_not_leak_metadata);
    RUN_TEST(hat_buttons_press_properly);
    RUN_TEST(stick_buttons_press_properly);
    RUN_TEST(multiple_button_presses_different);
    RUN_TEST(multiple_button_presses_same);
    RUN_TEST(multiple_button_presses_hat);
    RUN_TEST(multiple_button_presses_l_stick);
    RUN_TEST(multiple_button_presses_r_stick);
    RUN_TEST(multiple_button_presses_same_one_different_interleave);
    RUN_TEST(can_press_all_buttons);
    RUN_TEST(raw_updates_cancel_timers);
    RUN_TEST(raw_stick_updates_cancel_only_stick);
    RUN_TEST(deg_mag_updates_properly);
    RUN_TEST(full_magnitude_near_positive_axes_does_not_overflow);
    RUN_TEST(deg_mag_stick_press_properly);
    RUN_TEST(resets_controller);
    RUN_TEST(reset_preserves_callback_and_notifies);
    return UNITY_END();
}
