/*
 * SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdlib.h>
#include "class/hid/hid.h"
#include "device/usbd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "controller-state/controller-state.c"

#define TUSB_DESC_TOTAL_LEN      (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_INOUT_DESC_LEN)

enum {
    HORI_POKKEN_REPORT_DESCRIPTOR_LEN = 90,

    HORI_POKKEN_BUTTON_COUNT = 13,
    HORI_POKKEN_BUTTON_PADDING_BITS = 3,
    HORI_POKKEN_HAT_BITS = 4,
    HORI_POKKEN_AXIS_COUNT = 4,
    HORI_POKKEN_AXIS_BITS = 8,
    HORI_POKKEN_VENDOR_INPUT_COUNT = 1,
    HORI_POKKEN_VENDOR_OUTPUT_COUNT = 8,

    HORI_POKKEN_BUTTON_LOGICAL_MAX = 1,
    HORI_POKKEN_HAT_LOGICAL_MAX = 7,
    HORI_POKKEN_HAT_PHYSICAL_MAX_DEGREES = 315,
    HORI_POKKEN_AXIS_LOGICAL_MAX = 255,

    HORI_POKKEN_UNIT_NONE = 0x00,
    HORI_POKKEN_UNIT_ENGLISH_ROTATION_DEGREES = 0x14,

    HORI_POKKEN_VENDOR_INPUT_USAGE = 0x20,
    HORI_POKKEN_VENDOR_OUTPUT_USAGE = 0x2621,
};

#define HORI_POKKEN_HID_REPORT_DESC() \
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), \
    HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD), \
    HID_COLLECTION(HID_COLLECTION_APPLICATION), \
        HID_LOGICAL_MIN(0), \
        HID_LOGICAL_MAX(HORI_POKKEN_BUTTON_LOGICAL_MAX), \
        HID_PHYSICAL_MIN(0), \
        HID_PHYSICAL_MAX(1), \
        HID_REPORT_SIZE(1), \
        HID_REPORT_COUNT(HORI_POKKEN_BUTTON_COUNT), \
        HID_USAGE_PAGE(HID_USAGE_PAGE_BUTTON), \
        HID_USAGE_MIN(1), \
        HID_USAGE_MAX(HORI_POKKEN_BUTTON_COUNT), \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_REPORT_COUNT(HORI_POKKEN_BUTTON_PADDING_BITS), \
        HID_INPUT(HID_CONSTANT), \
        HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), \
        HID_LOGICAL_MAX(HORI_POKKEN_HAT_LOGICAL_MAX), \
        HID_PHYSICAL_MAX_N(HORI_POKKEN_HAT_PHYSICAL_MAX_DEGREES, 2), \
        HID_REPORT_SIZE(HORI_POKKEN_HAT_BITS), \
        HID_REPORT_COUNT(1), \
        HID_UNIT(HORI_POKKEN_UNIT_ENGLISH_ROTATION_DEGREES), \
        HID_USAGE(HID_USAGE_DESKTOP_HAT_SWITCH), \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NULL_STATE), \
        HID_UNIT(HORI_POKKEN_UNIT_NONE), \
        HID_REPORT_COUNT(1), \
        HID_INPUT(HID_CONSTANT), \
        HID_LOGICAL_MAX_N(HORI_POKKEN_AXIS_LOGICAL_MAX, 2), \
        HID_PHYSICAL_MAX_N(HORI_POKKEN_AXIS_LOGICAL_MAX, 2), \
        HID_USAGE(HID_USAGE_DESKTOP_X), \
        HID_USAGE(HID_USAGE_DESKTOP_Y), \
        HID_USAGE(HID_USAGE_DESKTOP_Z), \
        HID_USAGE(HID_USAGE_DESKTOP_RZ), \
        HID_REPORT_SIZE(HORI_POKKEN_AXIS_BITS), \
        HID_REPORT_COUNT(HORI_POKKEN_AXIS_COUNT), \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_PAGE_N(HID_USAGE_PAGE_VENDOR, 2), \
        HID_USAGE(HORI_POKKEN_VENDOR_INPUT_USAGE), \
        HID_REPORT_COUNT(HORI_POKKEN_VENDOR_INPUT_COUNT), \
        HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
        HID_USAGE_N(HORI_POKKEN_VENDOR_OUTPUT_USAGE, 2), \
        HID_REPORT_COUNT(HORI_POKKEN_VENDOR_OUTPUT_COUNT), \
        HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE), \
    HID_COLLECTION_END

#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
static const char *TAG = "example";

/* Flag to indicate if the host has suspended the USB bus */
static bool suspended = false;
/* Flag of possibility to Wakeup Host via Remote Wakeup feature */
static bool wakeup_host = false;

const uint8_t hid_report_descriptor[] = {
    HORI_POKKEN_HID_REPORT_DESC()
};

_Static_assert(sizeof(hid_report_descriptor) == HORI_POKKEN_REPORT_DESCRIPTOR_LEN,
               "HORI Pokken HID report descriptor must be 90 bytes");

/**
 * @brief String descriptor
 */
const char *hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "HORI CO.,LTD.",      // 1: Manufacturer
    "POKKEN CONTROLLER",  // 2: Product
    "123456",             // 3: Serials, should use chip ID
    "Human Interface Device",  // 4: HID
};

/**
 * @brief Configuration descriptor
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, 0, 500),

    // Interface number, string index, boot protocol, report descriptor len, EP Out/In address, size & polling interval
    TUD_HID_INOUT_DESCRIPTOR(0, 0, false, sizeof(hid_report_descriptor), 0x02, 0x81, 64, 5),
};


/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    (void) instance;
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

/********* Application ***************/

void tud_suspend_cb(bool remote_wakeup_en)
{
    ESP_LOGI(TAG, "USB device suspended");
    suspended = true;
    if (remote_wakeup_en) {
        ESP_LOGI(TAG, "Remote wakeup available, press the button to wake up the Host");
        wakeup_host = true;
    } else {
        ESP_LOGI(TAG, "Remote wakeup not available");
    }
}

void tud_resume_cb(void)
{
    ESP_LOGI(TAG, "USB device resumed");
    suspended = false;
}

static void app_send_hid_demo(void)
{
    ControllerState_t controller;
    JoystickState_t buffer;

    init_controller(&controller);

    ESP_LOGI(TAG, "Sending HAT_TOP");
    tap_hat(&controller, HAT_TOP);
    vTaskDelay(pdMS_TO_TICKS(20));
    serialize_controller_state(&controller, &buffer);
    tud_hid_report(0, &buffer, sizeof(JoystickState_t));
    reset_controller(&controller);


    ESP_LOGI(TAG, "Sending HAT_RIGHT");
    tap_hat(&controller, HAT_RIGHT);
    vTaskDelay(pdMS_TO_TICKS(20));
    serialize_controller_state(&controller, &buffer);
    tud_hid_report(0, &buffer, sizeof(JoystickState_t));
    reset_controller(&controller);

    ESP_LOGI(TAG, "Sending HAT_BOTTOM");
    tap_hat(&controller, HAT_BOTTOM);
    vTaskDelay(pdMS_TO_TICKS(20));
    serialize_controller_state(&controller, &buffer);
    tud_hid_report(0, &buffer, sizeof(JoystickState_t));
    reset_controller(&controller);

    ESP_LOGI(TAG, "Sending HAT_LEFT");
    tap_hat(&controller, HAT_LEFT);
    vTaskDelay(pdMS_TO_TICKS(20));
    serialize_controller_state(&controller, &buffer);
    tud_hid_report(0, &buffer, sizeof(JoystickState_t));
    reset_controller(&controller);
}

void app_main(void)
{
    // Initialize button that will trigger HID reports
    const gpio_config_t boot_button_config = {
        .pin_bit_mask = BIT64(APP_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&boot_button_config));

    ESP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = NULL;
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.string_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);
#if (TUD_OPT_HIGH_SPEED)
    tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;
#endif // TUD_OPT_HIGH_SPEED

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    
    while (1) {
        ESP_LOGI(TAG, "Hello!");
        tud_task();
        if (tud_mounted()) {
            ESP_LOGI(TAG, "Mounted!");
            static bool send_hid_data = true;
            if (send_hid_data) {
                if (!suspended) {
                    ESP_LOGI(TAG, "Sending!");
                    app_send_hid_demo();
                } else {
                    if (wakeup_host) {
                        ESP_LOGI(TAG, "Waking up the Host");
                        tud_remote_wakeup();
                        wakeup_host = false;
                    } else {
                        ESP_LOGI(TAG, "USB Host remote wakeup is not available.");
                    }
                }
            }
            send_hid_data = !gpio_get_level(APP_BUTTON);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
