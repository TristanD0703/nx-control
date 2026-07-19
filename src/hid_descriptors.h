#pragma once

#include <stdlib.h>
#include "class/hid/hid.h"
#include "device/usbd.h"
#include "freertos/task.h"
#include "common/tusb_types.h"

#define USB_VID 0x0f0d
#define USB_PID 0x0092
#define USB_BCD 0x0200

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
    0x09, 0x39,        // Usage (Hat Swi
};


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

