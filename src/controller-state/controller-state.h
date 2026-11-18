#pragma once

#include <stdint.h>
#include <math.h>
#include "../timer/timer.h"

#define MAX_TIMER_COUNT 17

typedef struct {
    double_t x;
    double_t y;
} Pos;

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

typedef void (*ControllerUpdateCb_t)(JoystickState_t *controller);

typedef struct {
    JoystickState_t curr_state;
    AppTimerHandle_t timer_handles[MAX_TIMER_COUNT];
    uint32_t magic;
    ControllerUpdateCb_t on_update;
} ControllerState_t;

typedef struct {
    TimerMetadata_t meta; 
    ControllerState_t *controller;
    JoystickButtonState_t operation;
    uint8_t timer_index;
    AppTimerHandle_t handle;
} TimerInfo_t;

/**
 *  Initializes the controller such that no buttons are pressed and
 *  sticks are in the center.
 *
 *  Returns zero if init fails
  */
int init_controller(ControllerState_t *controller);

void scheduler_callback(TimerMetadata_t *meta);

/**
 * Stops pressing the button and cancels the timer(s) related to this button
 */
int release_button(
  ControllerState_t *controller, 
  JoystickButtonState_t *operation
);

/**
 * Creates an event where the controller will send a button press to the
 * Nintendo Switch based on the specified button type, value, and duration (in ms).
 * If the duration specified is 0, the button will instead be held until release_button
 * is called
 *
 * Any subsequent calls to this method while the same button is active make the timer
 * expire that many ms in the future (effectively creating a new timer)
  */
int press_button(
  ControllerState_t *controller, 
  JoystickButtonState_t *button
);

/**
 * Ensures all button press commands passed are called pressed at the same
 * time given the constraints of the controller API
 */
int press_button_combo(
  ControllerState_t *controller, 
  JoystickButtonState_t *buttons, 
  const uint8_t buttons_len
);

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
);

/*
 * Reinitializes the controller to it's initial state while cancelling
 * all pending operations
 */
void reset_controller(ControllerState_t *controller);

/**
 * Retrieves the current state of the controller that can be sent over the USB port
  */
void serialize_controller_state(
  const ControllerState_t *controller, 
  JoystickState_t *buffer
);

/**
 * Sets the stick state directly without automatically creating timers.
 * Use this if you're directly managing stick directions rather than
 * holding a specific direction
 */
void update_sticks_raw(
    ControllerState_t *controller, 
    const uint8_t x, 
    const uint8_t y, 
    const JoystickOperation_t which_stick
);

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
);

/**
 * Sets a timer duration to update the controller state for the left 
 * or right stick based on a degree and magnitude vector. 
 * Magnitude >= 1 is fully extended.
 */
void update_sticks_deg_mag(
    ControllerState_t *controller, 
    const double_t degrees, 
    float_t magnitude, 
    const JoystickOperation_t which_stick,
    uint32_t duration_ms
);

int is_controller_initialized(const ControllerState_t *controller);

/**
 * When the controller changes any of its state (whether a button
 * was pressed or released, or was reset), the callback provided 
 * will be called.
 */
void set_controller_update_cb(ControllerState_t *controller, ControllerUpdateCb_t cb);

