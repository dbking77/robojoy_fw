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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "robodrive_comm.h"
#include "robodrive_comm_client.h"
#include "gatt_client.h"
#include "analog.h"
#include "joy_kb.h"
#include "lvgl_robot_select.h"
#include "nvs.h"
#include "nvs_flash.h"


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

#define SLIDER_SAMPLE_COUNT 100
#define SLIDER_SAMPLE_PERIOD_US 100
#define SLIDER_SELECT_HIGH_COUNT 80
#define SLIDER_SELECT_EXIT_LOW_US 100000

static void SliderSampleNotify(void *arg)
{
    xTaskNotifyGive((TaskHandle_t)arg);
}

static bool SliderSwitchRequestsSelect(void)
{
    const esp_timer_create_args_t timer_args = {
        .callback = SliderSampleNotify,
        .arg = xTaskGetCurrentTaskHandle(),
        .name = "slider_sample",
    };
    esp_timer_handle_t timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &timer));
    ulTaskNotifyTake(pdTRUE, 0);
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, SLIDER_SAMPLE_PERIOD_US));

    int high_count = 0;
    for (int sample = 0; sample < SLIDER_SAMPLE_COUNT; ++sample)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));
        if (gpio_get_level(SLIDER_SW_GPIO))
        {
            ++high_count;
        }
    }

    ESP_ERROR_CHECK(esp_timer_stop(timer));
    ESP_ERROR_CHECK(esp_timer_delete(timer));
    return high_count > SLIDER_SELECT_HIGH_COUNT;
}

static void JoystickSelectLoop(struct robodrive_command *cmd)
{
    int adc_count = 0;
    int64_t slider_low_since_us = -1;

    JoyKb_Init();
    Lvgl_SetSelectMode(true);

    while (1)
    {
        ++adc_count;
        if (adc_count >= 5)
        {
            adc_count = 0;

            int r_sw = gpio_get_level(R_SW_GPIO);
            AdcContinuous_FillJoyStickOutput(cmd);
            //ESP_LOGI(ROBOJOY_TAG, "r_joy fwd %d, right %d", (int)cmd->r_joy_fwd, (int)cmd->r_joy_right);
            //ESP_LOGI(ROBOJOY_TAG, "l_joy fwd %d, right %d", (int)cmd->l_joy_fwd, (int)cmd->l_joy_right);
            JoyKb_Update(cmd->r_joy_fwd, cmd->r_joy_right, r_sw);
        }

        if (gpio_get_level(SLIDER_SW_GPIO) == 0)
        {
            int64_t now_us = esp_timer_get_time();
            if (slider_low_since_us < 0)
            {
                slider_low_since_us = now_us;
            }
            else if ((now_us - slider_low_since_us) >= SLIDER_SELECT_EXIT_LOW_US)
            {
                break;
            }
        }
        else
        {
            slider_low_since_us = -1;
        }

        {
            // raise the task priority of LVGL and/or reduce the handler period can improve the performance
            vTaskDelay(pdMS_TO_TICKS(10));
            // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
            lv_timer_handler();
        }

        AdcContinuous_Read();
    }

    Lvgl_SetSelectMode(false);
}

#define NVS_NAMESPACE "robojoy"
#define NVS_KEY_ROBOT_INDEX "robot_idx"

static void Nvs_Init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static bool Nvs_LoadRobotIndex(int* index)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK)
    {
        return false;
    }

    int32_t value = 0;
    err = nvs_get_i32(handle, NVS_KEY_ROBOT_INDEX, &value);
    nvs_close(handle);
    if (err != ESP_OK)
    {
        return false;
    }

    *index = (int)value;
    return true;
}

static void Nvs_StoreRobotIndex(int index)
{
    nvs_handle_t handle;
    ESP_ERROR_CHECK(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_i32(handle, NVS_KEY_ROBOT_INDEX, index));
    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

static void ControlLoop(struct robodrive_command *cmd) __attribute__((noreturn));

static void ControlLoop(struct robodrive_command *cmd)
{
    int adc_count = 0;
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

            AdcContinuous_FillJoyStickOutput(cmd);
            //ESP_LOGI(ROBOJOY_TAG, "lv_tick %u cycle_counter %d", (unsigned)lv_tick_get(), cycle_counter)
        }

        {
            // raise the task priority of LVGL and/or reduce the handler period can improve the performance
            vTaskDelay(pdMS_TO_TICKS(10));
            // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
            lv_timer_handler();
            ++cycle_counter;
        }

        bool send_cmd = RoboDriveCommClient_Update(esp_timer_get_time(), GattClient_IsReadyForComm(), cmd);
        if (send_cmd)
        {
            //ESP_LOGI(ROBOJOY_TAG, "Send command : seqnum %d key %d", (int)cmd.seqnum, (int)cmd.key);
            GattClient_Send((uint8_t*)cmd, sizeof(*cmd));
        }

        AdcContinuous_Read();
    }
}

void app_main(void)
{
    Nvs_Init();

    int previous_robot_index = -1;
    int loaded_index = 0;
    if (Nvs_LoadRobotIndex(&loaded_index))
    {
        LvglRobotSelect_SetSelectedIndex(loaded_index);
        previous_robot_index = LvglRobotSelect_GetSelectedIndex();
    }

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
    GattClient_Init();
    RoboDriveCommClient_Init();

    Lvgl_Example1();

    struct robodrive_command cmd;
    if (SliderSwitchRequestsSelect())
    {
        ESP_LOGE("MODE", "Entering select mode");
        JoystickSelectLoop(&cmd);

        int selected_index = LvglRobotSelect_GetSelectedIndex();
        if (selected_index != previous_robot_index)
        {
            Nvs_StoreRobotIndex(selected_index);
        }
    }
    ESP_LOGE("MODE", "Entering control mode");

    ControlLoop(&cmd);
}
