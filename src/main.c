#include <stdint.h>
#include <stdlib.h>
#include "class/hid/hid.h"
#include "device/usbd.h"
#include "app_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "portmacro.h"
#include "timer/timer.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "class/hid/hid_device.h"
#include "driver/gpio.h"
#include "controller-state/controller-state.h"
#include "hid_descriptors.h"
#include "nvs_flash.h"
#include "bluetooth/gap.h"


#undef configUSE_TIMERS
#define configUSE_TIMERS 1

#define APP_BUTTON (GPIO_NUM_0)

static const char *TAG = "[nintendo-switch-usb-controller]";
ControllerState_t universal_controller;

// TODO: Refactor all of this usb stuff into its own thing
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
    APP_LOGI(TAG, "USB device suspended");
    suspended = true;
    if (remote_wakeup_en) {
        APP_LOGI(TAG, "Remote wakeup available, press the button to wake up the Host");
        wakeup_host = true;
    } else {
        APP_LOGI(TAG, "Remote wakeup not available");
    }
}

void tud_resume_cb(void)
{
    APP_LOGI(TAG, "USB device resumed");
    suspended = false;
}

static void controller_task(void* arg)
{
    JoystickButtonState_t buttons_to_press[] = {
        {
          .op = HAT,
          .value_x = HAT_TOP,
          .value_y = 0,
          .duration_ms = 100
        },
        {
          .op = HAT,
          .value_x = HAT_RIGHT,
          .value_y = 0,
          .duration_ms = 100
        },
        {
          .op = HAT,
          .value_x = HAT_BOTTOM,
          .value_y = 0,
          .duration_ms = 100
        },
        {
          .op = HAT,
          .value_x = HAT_LEFT,
          .value_y = 0,
          .duration_ms = 100 
        },
    };
    uint8_t curr_button_i = 0;
    TickType_t last_update = 0; 

    while (1) {
        TickType_t curr_tick = xTaskGetTickCount();
        if (curr_tick - last_update >= pdMS_TO_TICKS(1000)) {
            press_button(&universal_controller, &buttons_to_press[curr_button_i]);
            curr_button_i = (curr_button_i + 1) % 4;
            last_update = curr_tick;
            APP_LOGI(TAG, "Updating controller state");
        }

        if (!tud_mounted() || !tud_hid_ready()) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        
        JoystickState_t curr_state;
        serialize_controller_state(&universal_controller, &curr_state);
        tud_hid_report(0, &curr_state, sizeof(JoystickState_t));
        vTaskDelay(pdMS_TO_TICKS(5));
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

    
    /* NVS flash initialization */
    int ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return;
    }

    const size_t string_desc_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]);

    APP_LOGI(TAG, "USB initialization");
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();

    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.string = hid_string_descriptor;
    tusb_cfg.descriptor.full_speed_config = hid_configuration_descriptor;
    tusb_cfg.descriptor.string_count = string_desc_count;


    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    APP_LOGI(TAG, "USB initialization DONE");
  
    init_controller(&universal_controller);
    xTaskCreate(controller_task, "Controller Task", 4096, NULL, 5, NULL);

    ret = setup_bluetooth(&universal_controller);
    if (ret != 0) {
        APP_LOGE(TAG, "Failed to initialize Bluetooth. code %d", ret);
        return;
    }

    create_bluetooth_task();
}
