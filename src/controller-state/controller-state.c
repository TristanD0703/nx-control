#include <stdint.h>
#include <string.h>
#include <math.h>
#include "controller-state.h"
#include "FreeRTOSConfig.h"
#include "esp_log.h"
#include "esp_log_level.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/timers.h"
#include "../timer/timer.h"

#define TAG "[controller-state]"

/**
 *  Initializes the controller such that no buttons are pressed and 
 *  sticks are in the center
  */
void init_controller(ControllerState_t *controller) {
    memset(controller, 0, sizeof(ControllerState_t));

    controller->curr_state = (JoystickState_t){ 
        .LX = STICK_CENTER, 
        .LY = STICK_CENTER, 
        .RX = STICK_CENTER, 
        .RY = STICK_CENTER,
        .HAT = HAT_CENTER 
    };
}

/**
 * Frees up all memory related to the controller button press's timer
 */
static void shutdown_timer(
    ControllerState_t *controller, 
    JoystickButtonState_t *operation, 
    TimerHandle_t handle
)  {
    if (!handle) {
        if (!operation->timer_index)
            return;

        handle = controller->timer_metadata[operation->timer_index];
    }
    
    xTimerStop(handle, 0);
    xTimerDelete(handle, 0);
    controller->timer_metadata[operation->timer_index] = NULL;
}

static void shutdown_timer_direct(TimerHandle_t handle) {
    xTimerStop(handle, 0);
    xTimerDelete(handle, 0);
}

void release_button(
  ControllerState_t *controller, 
  JoystickButtonState_t *operation
) {
    JoystickState_t *state = &controller->curr_state;
    switch (operation->op) {
        case LSTICK: {
            state->LX = STICK_CENTER;
            state->LY = STICK_CENTER;
            break;
        }
        case RSTICK: {
            state->RX = STICK_CENTER;
            state->RY = STICK_CENTER;
            break;
        }
        case HAT: {
            state->HAT = HAT_CENTER; 
            break;
        }
        case BUTTON: {
            state->Button &= ~operation->value_x;
            break;
        }
    }

    shutdown_timer(controller, operation, NULL);
}

static void scheduler_callback(TimerHandle_t timer) {
    if (!timer)
        return;

    ESP_LOGI(TAG, "Releasing button");
    TimerInfo_t *info = (TimerInfo_t *)pvTimerGetTimerID(timer);

    release_button(info->controller, &info->operation);
    free(info);
}



static int is_button_pressed_internal(
  const JoystickState_t *state, 
  const JoystickButtonState_t *button
) {
    switch (button->op) {
        case LSTICK: {
            return state->LX != STICK_CENTER || state->LY != STICK_CENTER;
        }
        case RSTICK: {
            return state->RX != STICK_CENTER || state->RY != STICK_CENTER;
        }
        case HAT: {
            return state->HAT == button->value_x;
        }
        case BUTTON: {
            return (state->Button & button->value_x) != 0;
        }
        default: return 0;
    }
}

/**
 *  Checks if a button is pressed based on the desired button state.
 *  If the operation is either L or R stick, returns true if the stick is not centered.
 *  If the operation is HAT or D-Pad, returns true if it's in that exact configuration
 *  (ex: It will not return desired state is HAT right and current state is top-right)
 *  If op is button, returns true if that button is currently pressed
*/
int is_button_pressed(
  const ControllerState_t *state, 
  const JoystickButtonState_t *button
) {
    return is_button_pressed_internal(&state->curr_state, button);
}

/**
 * Finds an available storage space for the timer, allocates memory for 
 * timer metadata, and starts the timer with callback
 *
 * returns > 0 if an operation failed
 * 1: no timer space available
 * 2: malloc failed
 * 3: could not create timer
 * 4: could not start timer
*/
static uint8_t create_button_timer(
  ControllerState_t *controller, 
  JoystickButtonState_t *button
) {
    uint8_t free_index = 0; 
    for (; free_index < MAX_TIMER_COUNT; free_index++) {
        if (controller->timer_metadata[free_index] == NULL)
            break; 
    }

    if (free_index >= MAX_TIMER_COUNT)
        return 1;

    TimerInfo_t *info = malloc(sizeof(TimerInfo_t));
    if (info == NULL)
        return 2;

    if (button->op != LSTICK || button->op != RSTICK) {
        button->value_y = 0;
    }

    info->operation = *button;
    info->controller = controller;
    info->timer_index = free_index;
    info->operation.timer_index = free_index;

    uint64_t durationTicks = pdMS_TO_TICKS(
        info->operation.duration_ms
    );
    
    TimerHandle_t curr_timer = xTimerCreate(
        "Button Timer", 
        durationTicks, 
        pdFALSE, 
        (void *)info, 
        scheduler_callback
    );

    if (curr_timer == NULL)
        return 3;

    controller->timer_metadata[free_index] = curr_timer;
    if (xTimerStart(curr_timer, 0) != pdPASS)
        return 4;
    return 0;
}

/**
 * ACTUALLY modifies the controller internal state based on the
 * button action
 */
static void set_controller_state(
  ControllerState_t *controller, 
  JoystickButtonState_t *button
) {
    switch (button->op) {
        case LSTICK: {
            controller->curr_state.LX = button->value_x;
            controller->curr_state.LY = button->value_y;
            return;
        }
        case RSTICK: {
            controller->curr_state.RX = button->value_x;
            controller->curr_state.RY = button->value_y;
            return;
        }
        case BUTTON: {
          controller->curr_state.Button |= button->value_x;
          return;
        }
        case HAT: {
          controller->curr_state.HAT = button->value_x;
        }
    }
}

/*
 * If a timer is currently running for a desired button press or
 * controller state modification, extends that timer by the specified
 * time in ms.
 */
static void modify_timer_by_desired_state(
    ControllerState_t *controller, 
    JoystickButtonState_t *button
) {
    if (button->op != LSTICK || button->op != RSTICK)
        button->value_y = 0;

    TimerInfo_t *matching_timer = NULL;
    TimerHandle_t matching_handle;

    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        TimerHandle_t curr_timer = controller->timer_metadata[i];
        TimerInfo_t *curr_info = (TimerInfo_t *)pvTimerGetTimerID(
            curr_timer
        );

        if (
            curr_info->operation.op == button->op 
            && curr_info->operation.value_x == button->value_x 
            && curr_info->operation.value_y == button->value_y
        ) {
            matching_timer = curr_info;
            matching_handle = curr_timer;
            break;
        }
    }

    if (!matching_timer)
        create_button_timer(controller, button);

    matching_timer->operation.duration_ms += button->duration_ms;
    xTimerChangePeriod(
        matching_handle, 
        pdMS_TO_TICKS(button->duration_ms), 
        0
    );
}

/**
 * Creates an event where the controller will send a button press to the
 * Nintendo Switch based on the specified button type, value, and duration (in ms).
 *
 * Any subsequent calls to this method while the same button is active will extend
 * its duration.
 *
 * duration < 0 will result in the button being held indefinitely until 
 * release_button is called on that same button
  */
void press_button(ControllerState_t *controller, JoystickButtonState_t *button) { 
    int button_pressed = is_button_pressed_internal(&controller->curr_state, button);
   
    if (!button_pressed) {
        set_controller_state(controller, button);
        create_button_timer(controller, button);
        ESP_LOGI(TAG, "Pressed button");
    } else {
        modify_timer_by_desired_state(controller, button);
        ESP_LOGI(TAG, "Extended button timer");
    }
}


void press_button_combo(
  ControllerState_t *controller, 
  JoystickButtonState_t *buttons, 
  const uint8_t buttons_len
) {
   for (uint8_t i = 0; i < buttons_len; i++) {
      press_button(controller, &buttons[i]);
   }
}

void reset_controller(ControllerState_t *controller) {
    init_controller(controller);
    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        shutdown_timer_direct(controller->timer_metadata[i]);
        controller->timer_metadata[i] = NULL;
    }
}

/**
 * Sets the stick state directly without automatically creating timers
 */
void update_sticks_raw(
    ControllerState_t *controller, 
    const uint8_t x, 
    const uint8_t y, 
    const JoystickOperation_t which_stick
) {
    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        TimerHandle_t handle = controller->timer_metadata[i];
        TimerInfo_t *info = (TimerInfo_t *)pvTimerGetTimerID(handle);

        if (info->operation.op == which_stick) {
            shutdown_timer_direct(handle);
            controller->timer_metadata[i] = NULL;
        }
    }
    
    switch (which_stick) {
        case RSTICK: {
            controller->curr_state.RX = x;
            controller->curr_state.RY = y;
            break;
        }
        case LSTICK: {
            controller->curr_state.LX = x;
            controller->curr_state.LY = y;
            break;
        }
        default: return;
    }
        
}

/**
 * Directly updates the controller state for the left or right stick
 * based on a degree and magnitude vector. 
 * Magnitude >= 1 is fully extended
 */
void update_sticks_deg_mag_raw(
    ControllerState_t *controller, 
    const double_t degrees, 
    float_t magnitude, 
    const JoystickOperation_t which_stick
) {
    if (magnitude > 1.0)
        magnitude = 1.0;

    double_t radians = (degrees * M_PI) / 180;

    uint8_t x = cos(radians) * magnitude;
    uint8_t y = sin(radians) * magnitude;

    update_sticks_raw(controller, x, y, which_stick);
}


/**
 * Sets the controller state for the left or right stick
 * for the provided amount of time in ms based on a degree and 
 * magnitude vector. Magnitude >= 1 is fully extended
 */
void update_sticks_deg_mag(
    ControllerState_t *controller, 
    const double_t degrees, 
    float_t magnitude, 
    const JoystickOperation_t which_stick,
    uint32_t duration_ms
) {
    if (magnitude > 1.0)
        magnitude = 1.0;

    double_t radians = (degrees * M_PI) / 180;

    uint8_t x = cos(radians) * magnitude;
    uint8_t y = sin(radians) * magnitude;

    JoystickButtonState_t button = {
        .op = which_stick,
        .duration_ms = duration_ms,
        .value_x = x,
        .value_y = y,
    };

    press_button(controller, &button);
}

/**
 * Retrieves the current state of the controller that can be sent over the USB port
  */
void serialize_controller_state(const ControllerState_t *controller, JoystickState_t *buffer) {
  *buffer = controller->curr_state;
}
