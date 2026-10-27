#pragma once

#include <stdbool.h>
#include <stdint.h>

void GattClient_Init();

void GattClient_SetRemoteName(const char* name);

void GattClient_Send(uint8_t* data, unsigned len);

bool GattClient_IsReadyForComm();

void GattClient_ReadRSSI();

/* Last RSSI sample in dBm, or 0 when the read failed. */
int GattClient_GetRSSI();

unsigned GattClient_GetConnectionCount();