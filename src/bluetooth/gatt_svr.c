// Controller Service
//    Current State (which buttons are pressed) -> Readable and Subscribeable
//
//    Button (interface to press buttons, and to reset controller) -> Writeable only
//    ** Only one writer allowed **

#include "gatt_svr.h"

#include "controller-state/controller-state.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "sdkconfig.h"
#include "services/gatt/ble_svc_gatt.h"
#include "../app_log.h"
#include <stdint.h>

#define TAG "[GATT_SERVICE]"

static ControllerState_t *connected_controller;

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

static uint16_t subscribed_connection_handles[CONFIG_BT_NIMBLE_MAX_CONNECTIONS] = { 0 };

static int controller_state_access_cb(
    uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg) 
{
    JoystickState_t state; 
    serialize_controller_state(connected_controller, &state);

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
    {
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            APP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            APP_LOGI(TAG,
                     "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        if (attr_handle != controller_state_chr_val_handle) {
            APP_LOGE(TAG, "Controller state attr read called but attr_handle does not match");
            return BLE_ATT_ERR_UNLIKELY;
        }

        int rc = os_mbuf_append(ctxt->om, &state, sizeof(JoystickState_t));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    default:
    {
        APP_LOGE(TAG, "Invalid access operation on controller state. op=%d", ctxt->op);
        return BLE_ATT_ERR_UNLIKELY;
    }
    }
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

static void receive_controller_update(JoystickState_t *new_state) {
    if (!new_state) {
        return;
    }

    ESP_LOGI(TAG, "broadcasting new controller state...");
    for (uint8_t i = 0; i < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
        if (subscribed_connection_handles[i] != BLE_HS_CONN_HANDLE_NONE) {
            int rc = ble_gatts_notify(
                subscribed_connection_handles[i], 
                controller_state_chr_val_handle
            );

            if (rc != 0) {
                ESP_LOGE(TAG, "failed to send controller update, error code: %d conn_handle", rc, subscribed_connection_handles[i]);
            } else {
                ESP_LOGI(TAG, "sent controller update conn_handle", rc, subscribed_connection_handles[i]);
            }
        }
    }
}

void receive_subscribe_event(struct ble_gap_event *event) {
    // Subscribe or unsusbscribe?
    if (event->subscribe.attr_handle != controller_state_chr_val_handle) {
        return;
    }
    if (event->subscribe.cur_notify == 1) {
        // Is it already subscribed?
        for (uint8_t i = 0; i < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
            if (subscribed_connection_handles[i] == event->subscribe.conn_handle) {
                return;
            }
        }

        // If not, set an empty slot in subscribers list
        for (uint8_t i = 0; i < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
            if (subscribed_connection_handles[i] == BLE_HS_CONN_HANDLE_NONE) {
                subscribed_connection_handles[i] = event->subscribe.conn_handle;
                return;
            }
        }
    } else {
        // Unsubscribe by marking matching conn handles as unsubscribed
        for (uint8_t i = 0; i < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
            if (subscribed_connection_handles[i] == event->subscribe.conn_handle) {
                subscribed_connection_handles[i] = BLE_HS_CONN_HANDLE_NONE;
            }
        }
    }
    
}
int gatt_svc_init(ControllerState_t *controller) {
    /* Local variables */
    int rc = 0;
    if(!is_controller_initialized(controller)) {
        rc = 1;
        APP_LOGE(TAG, "Controller modifiable by the GATT service is not initialized");
        return rc;
    }

    memset(subscribed_connection_handles, BLE_HS_CONN_HANDLE_NONE, sizeof(subscribed_connection_handles));

    connected_controller = controller;
    set_controller_update_cb(connected_controller, receive_controller_update);

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
