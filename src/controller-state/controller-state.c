#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>

#include "controller-state.h"
#include "../timer/timer.h"

#define TAG "[controller-state]"

const uint32_t MAGIC_VALUE = 0xDEADBEEF;

int is_controller_initialized(const ControllerState_t *state) {
    return state != NULL && state->magic == MAGIC_VALUE;
}

int init_controller(ControllerState_t *controller, const TimerInterface_t *timer) {
    if (!controller || !timer) {
        return 0;
    }

    memset(controller, 0, sizeof(ControllerState_t));
    controller->curr_state = (JoystickState_t){ 
        .LX = STICK_CENTER, 
        .LY = STICK_CENTER, 
        .RX = STICK_CENTER, 
        .RY = STICK_CENTER,
        .HAT = HAT_CENTER 
    };

    controller->timer = timer;
    controller->magic = MAGIC_VALUE;
    return 1;
}

static void shutdown_timer(
    ControllerState_t *controller, 
    JoystickButtonState_t *operation, 
    AppTimerHandle_t handle
)  {
    if (!handle) {
        handle = controller->timer_metadata[operation->timer_index];

        if (!handle)
          return;
    }
    
    controller->timer->cancel_timer(handle);
    controller->timer_metadata[operation->timer_index] = NULL;
}

int release_button(
  ControllerState_t *controller, 
  JoystickButtonState_t *operation
) {
    if (!is_controller_initialized(controller)) {
        return 0;
    }

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
        default: {
            return 0;
        }
    }

    shutdown_timer(controller, operation, NULL);
    return 1;
}

void scheduler_callback(TimerMetadata_t *meta) {
    TimerInfo_t *info = (TimerInfo_t *)meta;
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
            return button->value_x ? state->HAT == button->value_x : state->HAT != HAT_CENTER;
        }
        case BUTTON: {
            return (state->Button & button->value_x) != 0;
        }
        default: return 0;
    }
}

int is_button_pressed(
  const ControllerState_t *state, 
  const JoystickButtonState_t *button
) {
    return is_controller_initialized(state) && 
        is_button_pressed_internal(&state->curr_state, button);
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

    if (button->op != LSTICK && button->op != RSTICK) {
        button->value_y = 0;
    }

    info->operation = *button;
    info->controller = controller;
    info->timer_index = free_index;
    info->operation.timer_index = free_index;


    AppTimerHandle_t handle = controller->timer->create_and_start_timer(
        info->operation.duration_ms, 
        scheduler_callback,
        (TimerMetadata_t *)info
    );
    controller->timer_metadata[free_index] = handle;

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
    if (button->op != LSTICK && button->op != RSTICK)
        button->value_y = 0;

    TimerInfo_t *matching_timer = NULL;
    AppTimerHandle_t matching_handle = NULL;

    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        AppTimerHandle_t curr_timer = controller->timer_metadata[i];

        if (!curr_timer) {
            continue;
        }

        TimerInfo_t *curr_info = (TimerInfo_t *)controller->timer->get_timer_metadata(curr_timer);

        int is_centerable_button = curr_info->operation.op != BUTTON;  

        int button_matches = curr_info->operation.op == button->op
          && is_centerable_button 
          || (!is_centerable_button && curr_info->operation.value_x == button->value_x);


        if (button_matches) {
            matching_timer = curr_info;
            matching_handle = curr_timer;
            break;
        }
    }

    if (!matching_timer || !matching_handle) {
        create_button_timer(controller, button);
        return;
    }

    matching_timer->operation.duration_ms += button->duration_ms;
    controller->timer->change_timer_duration(matching_handle, button->duration_ms);
}

int press_button(ControllerState_t *controller, JoystickButtonState_t *button) { 
    if (!is_controller_initialized(controller)) {
        return 0;
    }

    int button_pressed = is_button_pressed_internal(&controller->curr_state, button);

    // Buttons that can have a "neutral" state are "pressed" if there aren't centered.
    // We'd like to modify that state instead of creating a new timer for it
    int is_centerable_button = button->op != BUTTON;  

    if (!button_pressed && !is_centerable_button) {
        create_button_timer(controller, button);
    } else {
        modify_timer_by_desired_state(controller, button);
    }

    set_controller_state(controller, button);

    return 1;
}


int press_button_combo(
  ControllerState_t *controller, 
  JoystickButtonState_t *buttons, 
  const uint8_t buttons_len
) {
    int is_success = 1; 
    for (uint8_t i = 0; i < buttons_len; i++) {
      if (!press_button(controller, &buttons[i]))
          is_success = 0;
    }
    return is_success;
}

void reset_controller(ControllerState_t *controller) {
    if (!is_controller_initialized(controller)) {
        init_controller(controller, controller->timer);
        return;
    }

    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        AppTimerHandle_t curr_timer = controller->timer_metadata[i];
        
        if (!curr_timer) {  
            continue;
        }

        controller->timer->cancel_timer(curr_timer);
        controller->timer_metadata[i] = NULL;
    }

    init_controller(controller, controller->timer);
}

void update_sticks_raw(
    ControllerState_t *controller, 
    const uint8_t x, 
    const uint8_t y, 
    const JoystickOperation_t which_stick
) {
    if (!is_controller_initialized(controller)) {
        return;
    }

    for (uint8_t i = 0; i < MAX_TIMER_COUNT; i++) {
        AppTimerHandle_t handle = controller->timer_metadata[i];

        if (!handle) {
            continue;
        }

        TimerInfo_t *info = (TimerInfo_t *)controller->timer->get_timer_metadata(handle); 

        if (info->operation.op == which_stick) {
            controller->timer->cancel_timer(handle);
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

    uint8_t x = cos(radians) * magnitude * 255;
    uint8_t y = sin(radians) * magnitude * 255;

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

