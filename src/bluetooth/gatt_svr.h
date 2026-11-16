#include "controller-state/controller-state.h"
#include "host/ble_gatt.h"
#include "host/ble_gap.h"

#include <stdint.h>
int gatt_svc_init(ControllerState_t *controller);
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
void receive_subscribe_event(struct ble_gap_event *event);
