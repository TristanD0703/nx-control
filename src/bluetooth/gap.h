#include "controller-state/controller-state.h"
#include <stdint.h>

int setup_bluetooth(ControllerState_t *connected_controller);
void create_bluetooth_task();
int run_advertisement(int32_t duration);
