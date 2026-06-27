#include <stdint.h>
#include <stdlib.h>
#include "class/hid/hid.h"
#include "device/usbd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "controller-state/controller-state.h"
#include "hid_descriptors.h"

#undef configUSE_TIMERS
#define configUSE_TIMERS 1

#define APP_BUTTON (GPIO_NUM_0)

static const char *TAG = "[nintendo-switch-usb-controller]";

static bool suspended = false;
static bool wakeup_host = false;


uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return desc_hid_report;
}

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
        /** Receives data from the switch such as rumble commands */
    }
}

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
    ControllerState_t controller;
    init_controller(&controller);

    JoystickButtonState_t buttons_to_press[] = {
        {
          .op = HAT,
          .value_x = HAT_TOP,
          .value_y = 0,
          .duration_ms = 10
        },
        {
          .op = HAT,
          .value_x = HAT_RIGHT,
          .value_y = 0,
          .duration_ms = 10
        },
        {
          .op = HAT,
          .value_x = HAT_BOTTOM,
          .value_y = 0,
          .duration_ms = 10
        },
        {
          .op = HAT,
          .value_x = HAT_LEFT,
          .value_y = 0,
          .duration_ms = 10
        },
    };
    uint8_t curr_button_i = 0;

    while (1) {
        if (xTaskGetTickCount() % pdMS_TO_TICKS(1000) == 0) {
            press_button(&controller, &buttons_to_press[curr_button_i]);
            curr_button_i = (curr_button_i + 1) % 4;
        }

        JoystickState_t curr_state;
        serialize_controller_state(&controller, &curr_state);
        if (!tud_mounted() || !tud_hid_ready()) {
            ESP_LOGI(TAG, "USB Device not mounted or ready...");
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        tud_hid_report(0, &curr_state, sizeof(JoystickState_t));


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


    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    ESP_LOGI(TAG, "USB initialization DONE");
    
    ESP_LOGI(TAG, "Hello!");
    vTaskDelay(pdMS_TO_TICKS(100));
    xTaskCreate(controller_task, "Controller Task", 4096, NULL, 5, NULL);
}
