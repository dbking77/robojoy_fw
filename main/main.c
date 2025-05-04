/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "ST7789.h"
#include "SD_SPI.h"
#include "RGB.h"
#include "LVGL_Example.h"

#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_prelude.h"

#include "esp_log.h"
#include "esp_check.h"
#define ROBOJOY_TAG "ROBOJOY"
#include "esp_timer.h"

#include "robodrive_comm.h"
#include "robodrive_comm_client.h"
#include "gatt_client.h"
#include "analog.h"
#include "joy_kb.h"


#define RPOS_X_GPIO 1
#define RPOS_Y_GPIO 0
#define LPOS_X_GPIO 3
#define LPOS_Y_GPIO 2
#define V_BATT_GPIO 4
#define L_SW_GPIO 12
#define R_SW_GPIO 23
#define SLIDER_SW_GPIO 20


void Gpio_Init()
{
    int analog_gpio[ADC_CHANNEL_COUNT] = {
        RPOS_X_GPIO, RPOS_Y_GPIO, LPOS_X_GPIO, LPOS_Y_GPIO, V_BATT_GPIO
    };
    for (int ii = 0; ii < ADC_CHANNEL_COUNT; ++ii)
    {
        int gpio = analog_gpio[ii];
        //ESP_ERROR_CHECK(gpio_set_pull_mode(gpio, GPIO_PULLUP_PULLDOWN));
        //ESP_ERROR_CHECK(gpio_set_pull_mode(gpio, GPIO_PULLDOWN_ONLY));
        ESP_ERROR_CHECK(gpio_set_direction(gpio, GPIO_MODE_INPUT));
    }

    ESP_ERROR_CHECK(gpio_set_direction(L_SW_GPIO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(R_SW_GPIO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(SLIDER_SW_GPIO, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(L_SW_GPIO, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(R_SW_GPIO, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(SLIDER_SW_GPIO, GPIO_PULLUP_ONLY));
}

extern int g_rssi;





void app_main(void)
{
    //Wireless_Init();
    Flash_Searching();
    RGB_Init();
    RGB_Example();
    SD_Init();                              // SD must be initialized behind the LCD
    LCD_Init();
    BK_Light(50);
    LVGL_Init();                            // returns the screen object
    Gpio_Init();

    //Analog_Init();
    AdcContinuous_Init();
    //JoyKb_Init();
    GattClient_Init();
    RoboDriveCommClient_Init();

    Lvgl_Example1();

    int adc_count = 0;

    struct robodrive_command cmd;
    int cycle_counter = 0;

    while (1)
    {
        ++adc_count;
        if (adc_count >= 5)
        {
            adc_count = 0;

            int r_sw = gpio_get_level(R_SW_GPIO);
            int l_sw = gpio_get_level(L_SW_GPIO);
            int slider_sw = gpio_get_level(SLIDER_SW_GPIO);
            float v_batt = AdcContinuous_GetBatteryVoltage();
            //ESP_LOGE("GPIO", "R_SW %d, L_SW %d, SLIDER %d  VBATT %f", r_sw, l_sw, slider_sw, v_batt);
            //ESP_LOGI("RSSI", "g_rssi %d", g_rssi);

            AdcContinuous_FillJoyStickOutput(&cmd);
            //ESP_LOGI(ROBOJOY_TAG, "r_joy fwd %d, right %d", (int)cmd.r_joy_fwd, (int)cmd.r_joy_right);
            //ESP_LOGI(ROBOJOY_TAG, "l_joy fwd %d, right %d", (int)cmd.l_joy_fwd, (int)cmd.l_joy_right);
            //ESP_LOGI(ROBOJOY_TAG, "lv_tick %u cycle_counter %d", (unsigned)lv_tick_get(), cycle_counter)
            //JoyKb_Update(cmd.r_joy_fwd, cmd.r_joy_right, r_sw);
        }

        {
            // raise the task priority of LVGL and/or reduce the handler period can improve the performance
            vTaskDelay(pdMS_TO_TICKS(10));
            // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
            lv_timer_handler();
            ++cycle_counter;

            //JoyKey key = JoyKb_GetAndClearKey();
            //if (key != JOY_KEY_NONE)
            //{
            //   ESP_LOGE("JOY_KEY", "key %d", key);
            //}
        }

        bool send_cmd = RoboDriveCommClient_Update(esp_timer_get_time(), GattClient_IsReadyForComm(), &cmd);
        if (send_cmd)
        {
            //ESP_LOGI(ROBOJOY_TAG, "Send command : seqnum %d key %d", (int)cmd.seqnum, (int)cmd.key);
            GattClient_Send((uint8_t*)&cmd, sizeof(cmd));
        }

        AdcContinuous_Read();
    }
}
