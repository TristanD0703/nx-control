#include "host/ble_gap.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_id.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00
#define BLE_GAP_APPEARANCE_GAMEPAD 0x03C4

static const int32_t ADVERTISING_DURATION_MS = 2000;
static const char *DEVICE_NAME = "Hello World!";

static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);
static int gap_event_callback(struct ble_gap_event *event, void *arg);



static int gap_event_callback(struct ble_gap_event *event, void *arg) {
}


/**
 * Sends beacon for all devices in range to know about this device
 */
int run_advertisement() {
    int rc = 0;

    // Configure how device appears to clients and transmission power level
    struct ble_hs_adv_fields advertising_fields = { 0 };
    advertising_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    advertising_fields.tx_pwr_lvl_is_present = 1;

    advertising_fields.appearance = BLE_GAP_APPEARANCE_GAMEPAD;
    advertising_fields.appearance_is_present = 1;

    advertising_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    advertising_fields.le_role_is_present = 1;

    struct ble_gap_adv_params advertising_params = { 0 };
    uint8_t address_type = 0; 
    uint8_t bt_address[6] = {0};

    // Configure BT Device address
    rc = ble_hs_util_ensure_addr(0);
    rc = ble_hs_id_infer_auto(0, &address_type);
    rc = ble_hs_id_copy_addr(address_type, bt_address, NULL);
    
    struct ble_hs_adv_fields response_fields = { 0 };

    response_fields.device_addr = bt_address;
    response_fields.device_addr_type = address_type;
    response_fields.device_addr_is_present = 1;

    advertising_params.conn_mode = BLE_GAP_CONN_MODE_DIR;
    advertising_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    // instruct BLE controller to advertise
    rc = ble_gap_adv_start(address_type, NULL, ADVERTISING_DURATION_MS, &advertising_params, gap_event_callback, NULL);
    return rc;
}

int gap_init() {
    int rc = 0;

    return rc;
}
