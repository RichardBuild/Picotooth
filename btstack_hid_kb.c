/* this file was modified from hog_keyboard_demo.c */
/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BLUEKITCHEN
 * GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at
 * contact@bluekitchen-gmbh.com
 *
 */

//try to do this without ringbuffer and just queue
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "ble/gatt-service/hids_device.h"

#include "inttypes.h"

// Fix collision between btstack and tinyusb
#define HID_REPORT_TYPE_INPUT   TUSB_HID_REPORT_TYPE_INPUT
#define HID_REPORT_TYPE_OUTPUT  TUSB_HID_REPORT_TYPE_OUTPUT
#define HID_REPORT_TYPE_FEATURE TUSB_HID_REPORT_TYPE_FEATURE
#define hid_report_type_t       tusb_hid_report_type_t

#undef HID_USAGE_PAGE_DESKTOP
#undef HID_USAGE_PAGE_SIMULATE
#undef HID_USAGE_PAGE_VIRTUAL_REALITY
#undef HID_USAGE_PAGE_SPORT
#undef HID_USAGE_PAGE_GAME
#undef HID_USAGE_PAGE_GENERIC_DEVICE
#undef HID_USAGE_PAGE_KEYBOARD
#undef HID_USAGE_PAGE_LED
#undef HID_USAGE_PAGE_BUTTON
#undef HID_USAGE_PAGE_ORDINAL
#undef HID_USAGE_PAGE_TELEPHONY
#undef HID_USAGE_PAGE_CONSUMER
#undef HID_USAGE_PAGE_DIGITIZER
#undef HID_USAGE_PAGE_PID
#undef HID_USAGE_PAGE_UNICODE
#undef HID_USAGE_PAGE_ALPHA_DISPLAY
#undef HID_USAGE_PAGE_MEDICAL
#undef HID_USAGE_PAGE_LIGHTING_AND_ILLUMINATION
#undef HID_USAGE_PAGE_MONITOR
#undef HID_USAGE_PAGE_POWER
#undef HID_USAGE_PAGE_BARCODE_SCANNER
#undef HID_USAGE_PAGE_SCALE
#undef HID_USAGE_PAGE_MSR
#undef HID_USAGE_PAGE_CAMERA
#undef HID_USAGE_PAGE_ARCADE
#undef HID_USAGE_PAGE_FIDO
#undef HID_USAGE_PAGE_VENDOR

#undef HID_USAGE_LED_NUM_LOCK
#undef HID_USAGE_LED_CAPS_LOCK
#undef HID_USAGE_LED_SCROLL_LOCK
#undef HID_USAGE_LED_COMPOSE
#undef HID_USAGE_LED_KANA
#undef HID_USAGE_LED_POWER
#undef HID_USAGE_LED_SHIFT

#include "bsp/board.h"
#include "tusb.h"

#undef HID_REPORT_TYPE_INPUT
#undef HID_REPORT_TYPE_OUTPUT
#undef HID_REPORT_TYPE_FEATURE
#undef hid_report_type_t
#include "pico/util/queue.h"
#include "hog_custom_keypad.h"
#define REPORT_ID 0x01
queue_t hid_keyboard_report_queue;
hci_con_handle_t con_handle = HCI_CON_HANDLE_INVALID;
// from USB HID Specification 1.1, Appendix B.1

const uint8_t hid_descriptor_keyboard_boot_mode[] = {

    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xa1, 0x01,                    // Collection (Application)

    0x85,  REPORT_ID,                   // Report ID 1

    // Modifier byte
    0x05, 0x07,                    //   Usage Page (Key Codes)
    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7,                    //   Usage Maxium (Keyboard Right GUI)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0x01,                    //   Logical Maximum (1)
    0x81, 0x02,                    //   Input (Data, Variable, Absolute)

    // Reserved byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

    // LED report + padding

    0x95, 0x05,                    //   Report Count (5)
    0x75, 0x01,                    //   Report Size (1)
    0x05, 0x08,                    //   Usage Page (LEDs)
    0x19, 0x01,                    //   Usage Minimum (Num Lock)
    0x29, 0x05,                    //   Usage Maxium (Kana)
    0x91, 0x02,                    //   Output (Data, Variable, Absolute)

    0x95, 0x01,                    //   Report Count (1)
    0x75, 0x03,                    //   Report Size (3)
    0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

    // Keycodes

    0x95, 0x06,                    //   Report Count (6)
    0x75, 0x08,                    //   Report Size (8)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0xff,                    //   Logical Maximum (1)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
    0x29, 0xff,                    //   Usage Maxium (Reserved)
    0x81, 0x00,                    //   Input (Data, Array)

    0xc0,                          // End collection
};

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint8_t battery = 100;
static uint8_t protocol_mode = HID_PROTOCOL_MODE_REPORT;

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name
    0x0d, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'H', 'I', 'D', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd',
    // 16-bit Service UUIDs
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 
                ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    // Appearance HID - Keyboard (Category 15, Sub-Category 1)
    0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC1, 0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

// Buffer for 30 characters
static uint8_t key_input_storage[30];//stores keyvalues for sending
static btstack_ring_buffer_t key_input_buffer;
// HID Report sending
enum {
    W4_INPUT,
    W4_CAN_SEND_FROM_BUFFER,
    W4_CAN_SEND_KEY_UP,
} state;

static bool send_report(void){
    struct 
    {
        uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
        uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
        uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
    } report_q;

    if (queue_try_remove(&hid_keyboard_report_queue, &report_q)) {
        uint8_t report[] = {report_q.modifier, 0, 
                    report_q.keycode[0], report_q.keycode[1], report_q.keycode[2],
                    report_q.keycode[3], report_q.keycode[4], report_q.keycode[5]};
        switch (protocol_mode){
        case 0:
            hids_device_send_boot_keyboard_input_report(con_handle, report, sizeof(report));
            return 1;
        case 1:
           hids_device_send_input_report(con_handle, report, sizeof(report));
           return 1;
        default:
            return 0;
        }
    }
    else{
        return 0;
    }
}
static void typing_can_send_now(void){
    switch (state){
        case W4_CAN_SEND_FROM_BUFFER:
            while (1){
                if(send_report()){
                    state = W4_CAN_SEND_KEY_UP;
                    hids_device_request_can_send_now_event(con_handle);
                    break;
                }
                else{
                    state = W4_INPUT;
                    break;
                }  
            }
            break;
        case W4_CAN_SEND_KEY_UP:
            if (!queue_is_empty(&hid_keyboard_report_queue)) {
                state = W4_CAN_SEND_FROM_BUFFER;
                hids_device_request_can_send_now_event(con_handle);
            } else {
                state = W4_INPUT;
            }
            break;
        default:
            break;
    }
}

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            con_handle = HCI_CON_HANDLE_INVALID;
            printf("Disconnected\n");
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("Just Works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            printf("Confirming numeric comparison: %"PRIu32"\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            printf("Display Passkey: %"PRIu32"\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case HCI_EVENT_HIDS_META:
            switch (hci_event_hids_meta_get_subevent_code(packet)){
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
                    printf("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));
                    break;
                case HIDS_SUBEVENT_BOOT_KEYBOARD_INPUT_REPORT_ENABLE:
                    con_handle = hids_subevent_boot_keyboard_input_report_enable_get_con_handle(packet);
                    printf("Boot Keyboard Characteristic Subscribed %u\n", hids_subevent_boot_keyboard_input_report_enable_get_enable(packet));
                    break;
                case HIDS_SUBEVENT_PROTOCOL_MODE:
                    protocol_mode = hids_subevent_protocol_mode_get_protocol_mode(packet);
                    printf("Protocol Mode: %s mode\n", hids_subevent_protocol_mode_get_protocol_mode(packet) ? "Report" : "Boot");
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    typing_can_send_now();
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}
int main()
{
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("cyw43_arch_init error\n");
        return 0;
    }
    //2. l2cap initialize
    l2cap_init();

    //3. setup SM: Display only
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    //4. setup ATT server
    att_server_init(profile_data, NULL, NULL);

    //5. setup battery service
    battery_service_server_init(battery);

    //6. setup device information service
    device_information_service_server_init();

    //7. setup HID Device service
    hids_device_init(0, hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode));

    //8. setup advertisements
    uint16_t adv_int_min = 0x0030;
    uint16_t adv_int_max = 0x0030;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);
       
    //9. register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    //10. register for SM events
    sm_event_callback_registration.callback = &packet_handler;
    sm_add_event_handler(&sm_event_callback_registration);

    //11. register for HIDS
    hids_device_register_packet_handler(packet_handler);

    hci_power_control(HCI_POWER_ON);
    queue_init_with_spinlock(&hid_keyboard_report_queue, sizeof(hid_keyboard_report_t), 10, spin_lock_claim_unused(true));
    board_init();
    if (!tuh_init(BOARD_TUH_RHPORT)) { 
        printf("tuh init\n");
        return 0;
    }  else {
        printf("tinyusb init successfully\n");
    }

    while(1) {
        if (con_handle == HCI_CON_HANDLE_INVALID) {
            continue;//this is invalid so the loop does not reach tuh_task()
        }
        // tinyusb host task
        tuh_task();
    }
    return 0;
}