// Controller Service
//    Current State (which buttons are pressed) -> Readable and Subscribeable
//
//    Button (interface to press buttons, and to reset controller) -> Writeable only
//    ** Only one writer allowed **

#include "gatt_svr.h"

#include "host/ble_uuid.h"
#include "services/gatt/ble_svc_gatt.h"
#include "../app_log.h"

#define TAG "[GATT_SERVICE]"

static int controller_state_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg);

static int button_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt, void *arg);

static const ble_uuid128_t attr_controller_state_uuid = 
    BLE_UUID128_INIT(0x2c, 0xa6, 0xb9, 0x69, 0xc5, 0x72, 0x4e, 0xd1, 
                     0x84, 0x87, 0xc5, 0x9f, 0x92, 0x59, 0xac, 0x31);
static uint16_t controller_state_chr_val_handle;

static const struct ble_gatt_chr_def controller_state_chr =  {
    .uuid = &attr_controller_state_uuid.u,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
    .access_cb = controller_state_access_cb,
    .val_handle = &controller_state_chr_val_handle
};

static const ble_uuid128_t attr_button_uuid = 
    BLE_UUID128_INIT(0x53, 0xe6, 0xcc, 0xd0, 0xa8, 0x27, 0x44, 0x69,
                     0x85, 0xf3, 0xdf, 0xc8, 0x70, 0x6f, 0xc7, 0x75);
static uint16_t button_val_handle;

static const struct ble_gatt_chr_def button_chr =  {
    .uuid = &attr_button_uuid.u,
    .flags = BLE_GATT_CHR_F_WRITE,
    .access_cb = button_access_cb,
    .val_handle = &button_val_handle
};

static const ble_uuid128_t controller_service_uuid = 
    BLE_UUID128_INIT(0x88, 0x40, 0x5c, 0x94, 0x54, 0x7e, 0x4e, 0x8e, 
                     0xb2, 0x6c, 0xbb, 0xbb, 0x93, 0x03, 0x92, 0xa2);

static const struct ble_gatt_svc_def controller_service = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &controller_service_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[]) { button_chr, controller_state_chr, { 0 } }
};

static const struct ble_gatt_svc_def gatt_services[] = {
  controller_service,
  { 0 }
};


static int controller_state_access_cb(
    uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
    return 0; 
}


static int button_access_cb(
    uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
    return 0; 
}

/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        APP_LOGI(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        APP_LOGI(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        APP_LOGI(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        APP_LOGE(TAG, "received invalid event.");
        break;
    }
}

int gatt_svc_init(void) {
    /* Local variables */
    int rc = 0;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_services);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_services);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
