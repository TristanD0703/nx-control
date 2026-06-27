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
#include "common/tusb_types.h"
#include "tusb.h"


#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
static const char *TAG = "example";

/* Flag to indicate if the host has suspended the USB bus */
static bool suspended = false;
/* Flag of possibility to Wakeup Host via Remote Wakeup feature */
static bool wakeup_host = false;

uint8_t const desc_hid_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)

    // 13 buttons
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x01,        // Logical Maximum (1)
    0x35, 0x00,        // Physical Minimum (0)
    0x45, 0x01,        // Physical Maximum (1)
    0x75, 0x01,        // Report Size (1)
    0x95, 0x0D,        // Report Count (13)
    0x05, 0x09,        // Usage Page (Button)
    0x19, 0x01,        // Usage Minimum (Button 1)
    0x29, 0x0D,        // Usage Maximum (Button 13)
    0x81, 0x02,        // Input (Data,Var,Abs)

    // 3 bits padding
    0x95, 0x03,        // Report Count (3)
    0x81, 0x01,        // Input (Const,Array,Abs)

    // Hat switch
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x25, 0x07,        // Logical Maximum (7)
    0x46, 0x3B, 0x01,  // Physical Maximum (315)
    0x75, 0x04,        // Report Size (4)
    0x95, 0x01,        // Report Count (1)
    0x65, 0x14,        // Unit (English Rotation, Degrees)
    0x09, 0x39,        // Usage (Hat Switch)
    0x81, 0x42,        // Input (Data,Var,Abs,Null)

    // 4 bits padding
    0x65, 0x00,        // Unit (None)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x01,        // Input (Const,Array,Abs)

    // X, Y, Z, Rz axes
    0x26, 0xFF, 0x00,  // Logical Maximum (255)
    0x46, 0xFF, 0x00,  // Physical Maximum (255)
    0x09, 0x30,        // Usage (X)
    0x09, 0x31,        // Usage (Y)
    0x09, 0x32,        // Usage (Z)
    0x09, 0x35,        // Usage (Rz)
    0x75, 0x08,        // Report Size (8)
    0x95, 0x04,        // Report Count (4)
    0x81, 0x02,        // Input (Data,Var,Abs)

    // Vendor input byte
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        // Usage (0x20)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data,Var,Abs)

    // Vendor output: 8 bytes
    0x0A, 0x21, 0x26,  // Usage (0x2621)
    0x95, 0x08,        // Report Count (8)
    0x91, 0x02,        // Output (Data,Var,Abs)

    0xC0               // End Collection
};

#define USB_VID 0x0f0d
#define USB_PID 0x0092
#define USB_BCD 0x0200

 /* @brief String descriptor
 */
const char *hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},  // 0: is supported language is English (0x0409)
    "HORI CO.,LTD.",      // 1: Manufacturer
    "POKKEN CONTROLLER",  // 2: Product
    "123456",             // 3: Serials, should use chip ID
    "Human Interface Device",  // 4: HID
};

enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};
#define EPNUM_HID_IN  0x81

#define EPNUM_HID_OUT  0x02
#define EPNUM_HID_IN   0x81

#define CONFIG_TOTAL_LEN \
    (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)
#define TUSB_DESC_CONFIG_ATT_BUS_POWER 0x80

uint8_t const hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(
        1,
        ITF_NUM_TOTAL,
        0,
        CONFIG_TOTAL_LEN,
        TUSB_DESC_CONFIG_ATT_BUS_POWER,
        500
    ),

    TUD_HID_INOUT_DESCRIPTOR(
        ITF_NUM_HID,
        0,
        HID_ITF_PROTOCOL_NONE,
        sizeof(desc_hid_report),
        EPNUM_HID_OUT,
        EPNUM_HID_IN,
        64,
        5
    )
};

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    // These are the fields you asked about:
    .idVendor           = 0x0f0d,
    .idProduct          = 0x0092,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,

    .bNumConfigurations = 0x01
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    (void) instance;
    return desc_hid_report;
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
void tud_hid_set_report_cb(
    uint8_t instance,
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t const *buffer,
    uint16_t bufsize
) {
    (void) instance;
    (void) report_id;

    if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize >= 8) {
        // buffer[0..7] is the vendor-defined output report.
        // You can ignore it unless the host expects rumble/LED behavior.
    }
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

static void controller_task(void* arg)
{
    while (1) {
        if (!tud_mounted() || !tud_hid_ready()) {
            ESP_LOGI(TAG, "USB Device not mounted or ready...");
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        ESP_LOGI(TAG, "");
        ControllerState_t controller;
        JoystickState_t buffer;

        init_controller(&controller);

        ESP_LOGI(TAG, "Sending HAT_TOP");
        tap_hat(&controller, HAT_TOP);
        vTaskDelay(pdMS_TO_TICKS(1000));
        serialize_controller_state(&controller, &buffer);
        tud_hid_report(0, &buffer, sizeof(JoystickState_t));
        reset_controller(&controller);


        ESP_LOGI(TAG, "Sending HAT_RIGHT");
        tap_hat(&controller, HAT_RIGHT);
        vTaskDelay(pdMS_TO_TICKS(1000));
        serialize_controller_state(&controller, &buffer);
        tud_hid_report(0, &buffer, sizeof(JoystickState_t));
        reset_controller(&controller);

        ESP_LOGI(TAG, "Sending HAT_BOTTOM");
        tap_hat(&controller, HAT_BOTTOM);
        vTaskDelay(pdMS_TO_TICKS(1000));
        serialize_controller_state(&controller, &buffer);
        tud_hid_report(0, &buffer, sizeof(JoystickState_t));
        reset_controller(&controller);

        ESP_LOGI(TAG, "Sending HAT_LEFT");
        tap_hat(&controller, HAT_LEFT);
        vTaskDelay(pdMS_TO_TICKS(1000));
        serialize_controller_state(&controller, &buffer);
        tud_hid_report(0, &buffer, sizeof(JoystickState_t));
        reset_controller(&controller);
    }
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

    const size_t string_desc_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);

    ESP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.string_count = string_desc_count;

#if (TUD_OPT_HIGH_SPEED)
    tusb_cfg.descriptor.high_speed_config = hid_configuration_descriptor;
#endif // TUD_OPT_HIGH_SPEED

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    
    ESP_LOGI(TAG, "Hello!");
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(controller_task, "Controller Task", 4096, NULL, 5, NULL);
}
