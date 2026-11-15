#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "services/gatt/ble_svc_gatt.h"

#include <stdint.h>
int gatt_svc_init(void);
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
