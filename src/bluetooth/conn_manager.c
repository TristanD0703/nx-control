#include "host/ble_gap.h"
#include "../app_log.h"
#include "gap.h"
#include "host/ble_hs.h"
#include <stdint.h>
#include <stdio.h>

#define TAG "[CONN_MANAGER]"
#define DEVICE_ADDR_MAX_LEN 24 

static void handle_connect(const uint16_t conn_handle);
static void handle_disconnect(struct ble_gap_event *event);
static inline void address_to_string(ble_addr_t *addr, char *output);

uint8_t connection_count = 0;
uint16_t connection_handles[CONFIG_BT_NIMBLE_MAX_CONNECTIONS] = { 0 };

void init_conn_manager() {
    memset(connection_handles, 0xff, sizeof(connection_handles));
}

// TODO: Device subscribed
int gap_event_callback(struct ble_gap_event *event, void *arg) {
    char *callback_type_name = "UNKNOWN";

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT: 
    {
        callback_type_name = "CONNECT";
        if (event->connect.status != 0) {
            APP_LOGW(TAG, "Connection failed.");
            break;
        }
        handle_connect(event->connect.conn_handle);
        break;
    }
    case BLE_GAP_EVENT_DISCONNECT:
    {
        callback_type_name = "DISCONNECT";
        handle_disconnect(event);
        break; 
    }
    case BLE_GAP_EVENT_ADV_COMPLETE:
    {
        callback_type_name = "ADV_COMPLETE";
        break; 
    }
    }

    if (!ble_gap_adv_active() && connection_count < CONFIG_BT_NIMBLE_MAX_CONNECTIONS) {
        run_advertisement(BLE_HS_FOREVER);
    }
    APP_LOGI(TAG, "GAP callback with event type %s", callback_type_name);
    return 0;
}


static void handle_connect(const uint16_t conn_handle) {
    if (connection_count >= CONFIG_BT_NIMBLE_MAX_CONNECTIONS) {
        APP_LOGW(TAG, "Tried to connect with a new device while connections are full!");
        return;
    }

    for (uint8_t i = 0; i < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
        const uint16_t conn = connection_handles[i];

        if (conn == BLE_HS_CONN_HANDLE_NONE) {
            connection_handles[i] = conn_handle;
            break;
        }
    }

    struct ble_gap_conn_desc conn_desc;
    ble_gap_conn_find(conn_handle, &conn_desc);

    char device_address[DEVICE_ADDR_MAX_LEN];
    address_to_string(&conn_desc.peer_id_addr, device_address);

    connection_count++;
    APP_LOGI(TAG, "Device successfully connected. addr=%s", device_address);
}

static void handle_disconnect(struct ble_gap_event *event) {
    for (uint8_t i = 0; i < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; i++) {
        const uint16_t conn = connection_handles[i];

        if (conn == event->disconnect.conn.conn_handle) {
            connection_handles[i] = BLE_HS_CONN_HANDLE_NONE;
            break;
        }
    }

    connection_count--;
}

static inline void address_to_string(ble_addr_t *addr, char *output) {
    snprintf(output, DEVICE_ADDR_MAX_LEN, "%d:%d:%d:%d:%d:%d", addr->val[0], addr->val[1], addr->val[2], addr->val[3], addr->val[4], addr->val[5]);
}
