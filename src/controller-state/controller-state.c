#include <string.h>
#include "controller-state.h"


/**
 *  Initializes the controller such that no buttons are pressed
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
 *  Sets the D-Pad direction to the specified direction.
 *  Note: Only adjacent D-Pad buttons can be pressed as a combo 
 *  (ex: top and right)
  */
void tap_hat(ControllerState_t *controller, JoystickHatState_t hat) {
    controller->curr_state.HAT = hat;
}

void tap_button(ControllerState_t *controller, JoystickButton_t button) {
    controller->curr_state.Button |= button;
}

void hold_button_combo(
  ControllerState_t *controller, 
  JoystickButton_t *buttons, 
  uint8_t buttons_len, 
  uint32_t duration_ms
);

void hold_hat(
  ControllerState_t *controller, 
  JoystickHatState_t hat, 
  uint32_t duration_ms
);

void hold_left_stick_pos(
  ControllerState_t *controller, 
  uint8_t position_x, 
  uint8_t position_y, 
  uint32_t duration_ms
);

void hold_right_stick_pos(
  ControllerState_t *controller, 
  uint8_t position_x, 
  uint8_t position_y, 
  uint32_t duration_ms
);

void reset_controller(ControllerState_t *controller) {
    init_controller(controller);
}

void update_controller(ControllerState_t *controller) {
    init_controller(controller);
}


void serialize_controller_state(ControllerState_t *controller, JoystickState_t *buffer) {
  *buffer = controller->curr_state;
}
