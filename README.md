# Picotooth
Picotooth turns a Raspberry Pi Pico W (or Pico 2 W) into a BLE keyboard adapter, allowing wired keyboards to communicate to host devices wirelessly via BLE.

This project implements BLE on a pre-existing project completed by [MCU Application Lab](https://www.youtube.com/@MCUAppPrototypeLab) and their [Bluetooth Classic implementation](https://www.youtube.com/watch?v=RnpVAqOmc8E).

## Features 
- **6KRO** - supports up to 6 simultaneous key presses plus modifier keys.
- **BLE support** - Lower power consumption compared to Bluetooth Classic.

## Current Limitations and Possible Future Work
- **Keyboard only** - but code can be modified to support mice and other HID devices.
- **No media key support** - current implementation only supports standard keyboard page usage. See [Notes on Consumer / Media Keys](#notes-on-consumer--media-keys) for details.
- **No deep sleep mode** - Due to the recent introduction of the Pico 2 / RP2350, deep sleep functionality has not been tested or implemented.

## High Level Overview
1. Initialize BLE and USB stacks and begin advertising.
2. Detect keyboard inputs and place them into queue for processing.
3. HID reports are pulled from the queue and transmitted using HID over GATT protocol. 

## Rationale
The Raspberry Pi Pico W and Pico 2 W have many advantages in this application over other microcontrollers. The Pico W and Pico 2 W have low Bluetooth power consumption while having USB host support built in. With the addition of deep sleep in the RP2350, the Pico 2 W is both power efficient and easy to set up. 

Other candidates include microcontrollers with built-in USB host and Bluetooth such as the ESP32-S3, but these controllers often have high Bluetooth power consumption. Controllers with very low Bluetooth power consumption such as the nRF52840 require an external USB host chip such as the MAX3421E which increases power consumption and complexity. 

## Hardware Requirements
- Raspberry Pi Pico W or Pico 2 W
- Micro-USB OTG to USB A adapter (male Micro-USB, female USB A)
- 5V power supply

## Getting Started
### Software
You can either build the program yourself or flash the microcontroller with the UF2 file provided. 

Instructions on how to build the program are found in the official [pico-sdk repo](https://github.com/raspberrypi/pico-sdk?tab=readme-ov-file).

### Hardware
You will need a female USB A interface to connect your keyboard to the Pico. There are 2 methods to achieve this. You can either connect a USB OTG adapter or use the test pads by connecting a USB A breakout board as follows: D+ to TP3 pad, D- to TP2 pad, GND to TP1 pad, and VCC to pin 1 (VBUS). 

Then you can power the Pico by connecting 5V to pin 1 (VBUS) and GND. 

### Pairing
Once the Pico is flashed and powered, the device will show up as "HID Keyboard" on your host device. 

## Notes on Consumer / Media Keys
What I tried / learned while attempting media key support:

- I used Wireshark to log HID report descriptors for the Consumer Page, and found that consumer reports vary greatly from keyboard to keyboard.
- While Consumer Page usages are defined and standardized in the HID standard, supporting different kinds of keyboards require a detailed parser which is not provided by TinyUSB in that complexity.
- Sending consumer reports is straightforward on the Bluetooth stack. All that is needed is to extend the HID descriptor to include a Consumer Page and use 'hids_device_send_input_report_for_id'. 
- The current code uses boot protocol. Supporting consumer reports would require switching to report protocol. 
- I tested a few existing parsers but didn't get a working solution before stopping work on this project

Debugging issues: Because the USB port is used for host functionality, plan a separate debugging method.  

## Source Code, Credits and Licensing
This repository is largely an edit and connection of two source code examples from the BTstack and TinyUSB libraries. These files are [hog_keyboard_demo.c](https://github.com/bluekitchen/btstack/blob/501e6d2b86e6c92bfb9c390bcf55709938e25ac1/example/hog_keyboard_demo.c) from the BTstack library and [hid_app.c](https://github.com/hathach/tinyusb/blob/12814620f5a8fd91877fce094e170c38a4031b9a/examples/host/cdc_msc_hid/src/hid_app.c) from the TinyUSB library. 

This project was largely an edit of a Bluetooth Classic implementation done by YouTube channel [MCU Application Lab](https://www.youtube.com/@MCUAppPrototypeLab) and used code from their blog. 

The edits and connecting code is under MIT license; however please note that any files that contain a BTstack license header (including modified example
files) are **not** licensed under the MIT License. Those files remain subject
to the BTstack license from BlueKitchen GmbH, which permits use and
modification only for non-commercial purposes unless you obtain a separate
commercial license from BlueKitchen.

For the exact terms, see the license header at the top of the BTstack-derived
source files and the original BTstack license provided by BlueKitchen GmbH.