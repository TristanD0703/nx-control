#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_id.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "../app_log.h"
#include "gap.h"

#define TAG "[GAP]"
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00
#define BLE_GAP_APPEARANCE_GAMEPAD 0x03C4

// static const int32_t ADVERTISING_DURATION_MS = 2000;
static const char *DEVICE_NAME = "Hello World!";

// Library methods
void ble_store_config_init(void);

// internal methods
int run_advertisement(int32_t duration);
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static int gap_event_callback(struct ble_gap_event *event, void *arg);
static int retrieve_and_remember_address();
static int store_advertisement_configuration();
static void run_bluetooth();
static void bluetooth_task(void *args);

// Internal state vars
static uint8_t address_type = 0; 
static uint8_t bt_address[6] = {0};
static struct ble_hs_adv_fields advertising_fields = { 0 };
static struct ble_gap_adv_params advertising_params = { 0 };
static struct ble_hs_adv_fields response_fields = { 0 };

// TODO: Configure Bluetooth stack (before doing nimble_port_run())
// Start nvs storage (could move later if we use nvs elsewhere)
// Configure callbacks
// Set device name
int setup_bluetooth() {
    int rc = 0;
    ble_store_config_init();

    // Initialize nimble
    rc = nimble_port_init();
    if (rc != ESP_OK) {
        APP_LOGE(TAG, "nimble port init failed with code %d", rc);
        return rc;
    }

    // register BLE service and configure callback, name and appearance
    ble_svc_gap_init();
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        APP_LOGE(TAG, "failed to set device name to %s, error code: %d",
                 DEVICE_NAME, rc);
        return rc;
    }

    rc = ble_svc_gap_device_appearance_set(BLE_GAP_APPEARANCE_GAMEPAD);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set device appearance, error code: %d", rc);
    }

    // TODO: Also configure GATT when we add that
    return rc;
}

void create_bluetooth_task() {
    nimble_port_freertos_init(bluetooth_task);
}

static void bluetooth_task(void *args) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// TODO: Configure Bluetooth stack (after starting nimble port) (on_stack_sync)
//
// Discover and configure Bluetooth address 
// Configure advertising data to be sent
// Run advertising after configuration
static void run_bluetooth() {
    if (retrieve_and_remember_address())
        return;

    if(store_advertisement_configuration())
        return;

    if (run_advertisement(BLE_HS_FOREVER)) {
        return;
    }
}

static void on_stack_reset(int reason) {
    APP_LOGE(TAG, "BLE stack reset!! code %d", reason);
}

static void on_stack_sync() {
    run_bluetooth();
}

// TODO: Handle GAP events
// Connect (success)
// Connect (failure)
// Disconnect
// Advertising complete
// TODO: Device subscribed
// TODO: Connection updated
static int gap_event_callback(struct ble_gap_event *event, void *arg) {
    char * callback_type_name = "UNKNOWN";

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT: 
    {
        callback_type_name = "CONNECT";
        if (event->connect.status != 0) {
            APP_LOGW(TAG, "Connection failed.");
        }
        break;
    }
    case BLE_GAP_EVENT_DISCONNECT:
    {
        callback_type_name = "DISCONNECT";
        break; 
    }
    case BLE_GAP_EVENT_ADV_COMPLETE:
    {
        callback_type_name = "ADV_COMPLETE";
        break; 
    }
    }

    if (!ble_gap_adv_active()) {
        run_advertisement(BLE_HS_FOREVER);
    }
    APP_LOGI(TAG, "GAP callback with event type %s", callback_type_name);
    return 0;
}

/**
 * Sends beacon for all devices in range to know about this device
 */
int run_advertisement(int32_t duration) {
    int rc = 0;

    ble_addr_t *advertising_target = NULL;
    void *cb_args = NULL;

    // instruct BLE controller to advertise
    rc = ble_gap_adv_start(
        address_type, 
        advertising_target, 
        duration, 
        &advertising_params, 
        gap_event_callback, 
        cb_args
    );

    if (rc != BLE_ERR_SUCCESS) {
        APP_LOGE(TAG, "Begin advertising failed with code %d", rc);
    }
    return rc;
}

static int retrieve_and_remember_address() {
    int rc = 0;
    rc = ble_hs_util_ensure_addr(0);
    if (rc != BLE_ERR_SUCCESS) {
        APP_LOGE(TAG, "Ensure address failed with code %d", rc);        
        return rc;
    }

    rc = ble_hs_id_infer_auto(0, &address_type);
    if (rc != BLE_ERR_SUCCESS) {
        APP_LOGE(TAG, "Infer address failed with code %d", rc);        
        return rc;
    }

    rc = ble_hs_id_copy_addr(address_type, bt_address, NULL);
    if (rc != BLE_ERR_SUCCESS) {
        APP_LOGE(TAG, "Copy address failed with code %d", rc);
    }
    return rc;
}

static int store_advertisement_configuration() {
    int rc = 0;
    // Configure how device appears to clients and transmission power level
    advertising_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP; // Disable bluetooth classic

    advertising_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    advertising_fields.tx_pwr_lvl_is_present = 1;

    advertising_fields.appearance = BLE_GAP_APPEARANCE_GAMEPAD;
    advertising_fields.appearance_is_present = 1;

    advertising_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    advertising_fields.le_role_is_present = 1;

    // Set device name
    const char* name = ble_svc_gap_device_name();
    if (name == NULL) {
        name = DEVICE_NAME;
    }

    advertising_fields.name = (uint8_t *)name;
    advertising_fields.name_len = strlen(name);
    advertising_fields.name_is_complete = 1;

    response_fields.device_addr = bt_address;
    response_fields.device_addr_type = address_type;
    response_fields.device_addr_is_present = 1;

    advertising_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    advertising_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_set_fields(&advertising_fields);
    if (rc != BLE_ERR_SUCCESS) {
        APP_LOGE(TAG, "Advertising set fields failed with code %d", rc);        
        return rc;
    }
    rc = ble_gap_adv_rsp_set_fields(&response_fields);
    if (rc != BLE_ERR_SUCCESS) {
        APP_LOGE(TAG, "Advertising response set fields failed with code %d", rc);        
    }

    return rc;
}

