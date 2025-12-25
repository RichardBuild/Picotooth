cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(picow_bt_hid_keyboard C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(btstack_hid_kb
      btstack_hid_kb.c
      tusb_hid.c
      )

pico_set_program_name(btstack_hid_kb "btstack_hid_kb")
pico_set_program_version(btstack_hid_kb "0.1")

pico_enable_stdio_uart(btstack_hid_kb 1)
pico_enable_stdio_usb(btstack_hid_kb 0)

# Add the standard library to the build
target_link_libraries(btstack_hid_kb
        pico_stdlib
        #pico_util
        pico_cyw43_arch_none
        pico_btstack_cyw43
        pico_btstack_ble)

# Add the standard include files to the build
target_include_directories(btstack_hid_kb PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)
pico_btstack_make_gatt_header(btstack_hid_kb PRIVATE "${CMAKE_CURRENT_LIST_DIR}/hog_keyboard.gatt")
# Add any user requested libraries   ----------
target_link_libraries(btstack_hid_kb
        tinyusb_host 
        tinyusb_board
        )

pico_add_extra_outputs(btstack_hid_kb)

