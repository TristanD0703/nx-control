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
#include "controller-state/controller-state.h"
#include "hid_descriptors.h"


#define APP_BUTTON (GPIO_NUM_0) // Use BOOT signal by default
static const char *TAG = "example";

/* Flag to indicate if the host has suspended the USB bus */
static bool suspended = false;
/* Flag of possibility to Wakeup Host via Remote Wakeup feature */
static bool wakeup_host = false;


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
