#include "analog.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_continuous.h"

#define ADC_TAG "ADC"

#define RPOS_X_ADC_CH ADC_CHANNEL_1
#define RPOS_Y_ADC_CH ADC_CHANNEL_0
#define LPOS_X_ADC_CH ADC_CHANNEL_3
#define LPOS_Y_ADC_CH ADC_CHANNEL_2
#define V_BATT_ADC_CH ADC_CHANNEL_4
#define ADC_READ_LEN (ADC_CHANNEL_COUNT * SOC_ADC_DIGI_DATA_BYTES_PER_CONV * 4)


static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_continuous_handle_t adc_handle = NULL;

struct analog_ch_avg
{
    uint32_t sum;
    uint32_t count;
};

static struct analog_ch_avg analog_ch_avgs[ADC_CHANNEL_COUNT];


void Analog_Init()
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RPOS_X_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, RPOS_Y_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LPOS_X_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, LPOS_Y_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, V_BATT_ADC_CH, &config));
}

void Analog_Read()
{
    int rpos_x, rpos_y, lpos_x, lpos_y, v_batt;
    uint64_t start_us = esp_timer_get_time();
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, RPOS_X_ADC_CH, &rpos_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, RPOS_Y_ADC_CH, &rpos_y));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LPOS_X_ADC_CH, &lpos_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, LPOS_Y_ADC_CH, &lpos_y));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, V_BATT_ADC_CH, &v_batt));
    uint64_t stop_us = esp_timer_get_time();

    ESP_LOGI(ADC_TAG, "Read time %u usec : RPosX %d, RPosY %d, LPosX %d, LPosY %d, VBatt %d",
        (unsigned)(stop_us-start_us), rpos_x, rpos_y, lpos_x, lpos_y, v_batt);
}

void AdcContinuous_Init()
{
    //ESP_LOGI(ADC_TAG, "SOC_ADC_DIGI_DATA_BYTES_PER_CONV %d", SOC_ADC_DIGI_DATA_BYTES_PER_CONV);

    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = ADC_READ_LEN*4,
        .conv_frame_size = ADC_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 1000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };

    adc_channel_t channels[ADC_CHANNEL_COUNT] = {
        RPOS_X_ADC_CH,
        RPOS_Y_ADC_CH,
        LPOS_X_ADC_CH,
        LPOS_Y_ADC_CH,
        V_BATT_ADC_CH,
    };

    adc_digi_pattern_config_t adc_pattern[ADC_CHANNEL_COUNT] = {0};
    dig_cfg.pattern_num = ADC_CHANNEL_COUNT;
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        adc_pattern[i].atten = ADC_ATTEN_DB_12;
        adc_pattern[i].channel = channels[i] & 0x7;
        adc_pattern[i].unit = ADC_UNIT_1;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH; //ADC_BITWIDTH_DEFAULT; //SOC_ADC_DIGI_MAX_BITWIDTH;

        ESP_LOGI(ADC_TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(ADC_TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(ADC_TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    ESP_ERROR_CHECK(adc_continuous_start(handle));

    adc_handle = handle;
}

void AdcContinuous_Read()
{
    uint8_t adc_result_buf[ADC_READ_LEN];
    uint32_t actual_read_len = 0;
    uint32_t timeout_ms = 0;
    //uint64_t start_us = esp_timer_get_time();
    esp_err_t ret = adc_continuous_read(adc_handle, adc_result_buf, ADC_READ_LEN, &actual_read_len, timeout_ms);
    if (ret != ESP_OK)
    {
        return;
    }
    //uint64_t stop_us = esp_timer_get_time();
    //ESP_LOGI(ADC_TAG, "Read %u bytes in %u usec", (unsigned)actual_read_len, (unsigned)(stop_us-start_us));
    for (int offset = 0; offset < actual_read_len; offset += SOC_ADC_DIGI_RESULT_BYTES) {
        adc_digi_output_data_t *sample = (adc_digi_output_data_t*)&adc_result_buf[offset];
        uint32_t channel = sample->type2.channel;
        uint32_t value = sample->type2.data;
        struct analog_ch_avg* ch_avg = NULL;
        switch (channel)
        {
        case RPOS_X_ADC_CH:
            ch_avg = &analog_ch_avgs[0];
            break;
        case RPOS_Y_ADC_CH:
            ch_avg = &analog_ch_avgs[1];
            break;
        case LPOS_X_ADC_CH:
            ch_avg = &analog_ch_avgs[2];
            break;
        case LPOS_Y_ADC_CH:
            ch_avg = &analog_ch_avgs[3];
            break;
        case V_BATT_ADC_CH:
            ch_avg = &analog_ch_avgs[4];
            break;
        }
        if (ch_avg != NULL)
        {
            ++ch_avg->count;
            ch_avg->sum += value;
        }
    }
}

float getAverage(struct analog_ch_avg* ch_avg)
{
    float avg = ch_avg->count ? ((float)ch_avg->sum) / ch_avg->count : 0.0;
    return avg;
}

int getRatio(struct analog_ch_avg* ch_avg, float center, float range)
{
    int ratio = 0;
    if (ch_avg->count)
    {
        float avg = ch_avg->count ? ((float)ch_avg->sum) / ch_avg->count : 0.0;
        ratio = (int)(100.0f * 2.0f * (avg - center) / range);
        if (ratio > 100)
        {
            ratio = 100;
        }
        if (ratio < -100)
        {
            ratio = -100;
        }
    }
    ch_avg->sum = 0;
    ch_avg->count = 0;
    return ratio;
}



void AdcContinuous_FillJoyStickOutput(struct robodrive_command* cmd)
{
    //ESP_LOGI(ADC_TAG, "r_joy fwd %f, right %f", getAverage(&analog_ch_avgs[0]), getAverage(&analog_ch_avgs[1]));
    //ESP_LOGI(ADC_TAG, "l_joy fwd %f, right %f", getAverage(&analog_ch_avgs[2]), getAverage(&analog_ch_avgs[3]));
    cmd->r_joy_fwd = getRatio(&analog_ch_avgs[0], 1455.0f, -2456.f);  // 191 2646
    cmd->r_joy_right = getRatio(&analog_ch_avgs[1], 1425.0f, 2456.f); // 2646 190
    cmd->l_joy_fwd = getRatio(&analog_ch_avgs[2], 1410.0f, 2456.f);  // 2646 190
    cmd->l_joy_right = getRatio(&analog_ch_avgs[3], 1415.0f, -2456.f); // 190 2646
}


float AdcContinuous_GetBatteryVoltage()
{
    struct analog_ch_avg* ch_avg = &analog_ch_avgs[4];
    float avg = ch_avg->count ? ((float)ch_avg->sum) / ch_avg->count : 0.0;
    ch_avg->count = 0;
    ch_avg->sum = 0;
    // 10k / (10k + 10k) = 1/2
    // with 11db attenuation 2.5V = 4098
    float voltage = avg * (2.5 / 4098.0f * 2.0);
    return voltage;
}