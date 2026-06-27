#pragma once

#include "freertos/idf_additions.h"
#include <stdint.h>

#define MAX_TIMER_COUNT 20

typedef enum {
	SWITCH_Y       = 0x01,
	SWITCH_B       = 0x02,
	SWITCH_A       = 0x04,
	SWITCH_X       = 0x08,
	SWITCH_L       = 0x10,
	SWITCH_R       = 0x20,
	SWITCH_ZL      = 0x40,
	SWITCH_ZR      = 0x80,
	SWITCH_MINUS   = 0x100,
	SWITCH_PLUS    = 0x200,
	SWITCH_LCLICK  = 0x400,
	SWITCH_RCLICK  = 0x800,
	SWITCH_HOME    = 0x1000,
	SWITCH_CAPTURE = 0x2000,
} JoystickButton_t;

typedef enum {
   HAT_TOP          = 0x00,
   HAT_TOP_RIGHT    = 0x01,
   HAT_RIGHT        = 0x02,
   HAT_BOTTOM_RIGHT = 0x03,
   HAT_BOTTOM       = 0x04,
   HAT_BOTTOM_LEFT  = 0x05,
   HAT_LEFT         = 0x06,
   HAT_TOP_LEFT     = 0x07,
   HAT_CENTER       = 0x08,
} JoystickHatState_t;

typedef enum {
   STICK_MIN        = 0,
   STICK_CENTER     = 128,
   STICK_MAX        = 255,
} JoystickDirectionPositions_t;

typedef enum {
    BUTTON,
    HAT,
    LSTICK,
    RSTICK,
} JoystickOperation_t;

typedef struct {
    JoystickOperation_t op;
    uint16_t value_x;
    uint16_t value_y; // Only defined if op is L/RSTICK
    uint32_t duration_ms;
    uint8_t timer_index;
} JoystickButtonState_t;

typedef struct {
	uint16_t Button; // 16 buttons; see JoystickButtons_t for bit mapping
	uint8_t  HAT;    // HAT switch; one nibble w/ unused nibble
	uint8_t  LX;     // Left  Stick X
	uint8_t  LY;     // Left  Stick Y
	uint8_t  RX;     // Right Stick X
	uint8_t  RY;     // Right Stick Y
	uint8_t  VendorSpec;
} JoystickState_t;

typedef struct {
    JoystickState_t curr_state;
    TimerHandle_t timer_metadata[MAX_TIMER_COUNT];
} ControllerState_t;

void init_controller(ControllerState_t *controller);

void press_button(
  ControllerState_t *controller, 
  JoystickButtonState_t *button
);

void press_button_combo(
  ControllerState_t *controller, 
  const JoystickButtonState_t *buttons, 
  const uint8_t buttons_len
);

int is_button_pressed(
  const ControllerState_t *state, 
  const JoystickButtonState_t *button
);

void reset_controller(ControllerState_t *controller);

void serialize_controller_state(
  const ControllerState_t *controller, 
  JoystickState_t *buffer
);
