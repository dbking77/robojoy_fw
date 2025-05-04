#pragma once

#include <stdint.h>
#include <stdbool.h>

#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0
#define ROBODRIVE_MAGIC 0xDE
#define ROBODRIVE_BLE_ADV_NAME "ROBODRIVE_2025"

struct robodrive_header
{
    uint8_t magic;       // Magic number 0xEA

    // MSB nibble major, LSB nibble minor
    // Major number should match for FW to be compatible
    uint8_t fw_version;
};

extern struct robodrive_header default_robodrive_hdr;

struct robodrive_command
{
    struct robodrive_header hdr;

    // Counts up each time, response should include seqnum
    uint8_t seqnum;

    // Should match previous response key for drive to be enabled
    // prevents robodrive from operation getting out-of-sync with drive
    uint8_t key;

    // bit0 = enable drive / servo
    // bit1 = right jostick button : 1 = pressed
    // bit2 = left joystick button : 1 = pressed
    uint8_t flags;

    // Right joystick
    int8_t r_joy_fwd;    // -100 to 100  : positive forward, negative backward
    int8_t r_joy_right;  // -100 to 100  :  positive right, negative left

    // Left joystick
    int8_t l_joy_fwd;    // -100 to 100  : positive forward, negative backward
    int8_t l_joy_right;  // -100 to 100  :  positive right, negative left
};

struct robodrive_response
{
    struct robodrive_header hdr;

    // Should match cmd seqnum use
    uint8_t seqnum;

    // key that joystick shoud use to
    uint8_t key;

    // bit 0 enabled
    // bit 1 motor fault
    // bit 2 battery voltage low
    // bit 3 timeout
    // bit 4 bad key
    uint8_t flags;

    // Battery level in percentage 0 to 100
    uint8_t batt_percent;

    // Seconds since last reboot , saturates at 255
    // Used to detect brown-outs / unexpected resets
    uint8_t cpu_uptime;

    // Seconds that motor has been operating
    uint8_t motor_uptime;
};

uint8_t get_fw_version_major(uint8_t fw_version);

uint8_t get_fw_version_minor(uint8_t fw_version);

bool robodrive_check_header(struct robodrive_header* hdr);