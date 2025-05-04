#pragma once

#include <stdbool.h>
#include <stdint.h>

void GattClient_Init();

void GattClient_Send(uint8_t* data, unsigned len);

bool GattClient_IsReadyForComm();

void GattClient_ReadRSSI();
int GattClient_GetRSSI();