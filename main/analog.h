#pragma once

#include <stdint.h>
#include "robodrive_comm.h"

#define ADC_CHANNEL_COUNT 5

void Analog_Init();
void Analog_Read();

void AdcContinuous_Init();
void AdcContinuous_Read();
void AdcContinuous_FillJoyStickOutput(struct robodrive_command* cmd);
float AdcContinuous_GetBatteryVoltage();
