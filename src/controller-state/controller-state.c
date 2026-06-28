#include <stdint.h>
#include <string.h>
#include "controller-state.h"
#include "FreeRTOSConfig.h"
#include "esp_log.h"
#include "esp_log_level.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/timers.h"

#define TAG "[controller-state]"

typedef struct {
    ControllerState_t *controller;
    JoystickButtonState_t operation;
    uint8_t timer_index;
} TimerInfo_t;

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
static void shutdown_timer(ControllerState_t *controller, JoystickButtonState_t *operation, TimerHandle_t handle)  {
    if (!handle)
        handle = controller->timer_metadata[operation->timer_index];

    xTimerStop(handle, 0);
    controller->timer_metadata[operation->timer_index] = NULL;
}

static void release_button(
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
            ESP_LOGI(TAG, "RELEASE HAT");
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

    info->operation = *button;
    info->controller = controller;
    info->timer_index = free_index;
    info->operation.timer_index = free_index;

    uint64_t durationTicks = pdMS_TO_TICKS(info->operation.duration_ms);
    
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
static void modify_timer_by_desired_state(const JoystickState_t *button) {
}
*/

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
        ESP_LOGI(TAG, "Pressing button currently activated not supported yet.");
    }
}


void press_button_combo(
  ControllerState_t *controller, 
  const JoystickButtonState_t *buttons, 
  const uint8_t buttons_len
) {
    
}

void reset_controller(ControllerState_t *controller) {
    init_controller(controller);
}

/**
 * Retrieves the current state of the controller that can be sent over the USB port
  */
void serialize_controller_state(const ControllerState_t *controller, JoystickState_t *buffer) {
  *buffer = controller->curr_state;
}
