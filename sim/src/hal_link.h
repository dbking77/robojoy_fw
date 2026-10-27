#pragma once

#include <stdint.h>

#define HAL_LINK_RSSI_DEFAULT (-65)
#define HAL_LINK_CONN_DEFAULT 1u
#define HAL_LINK_BATT_DEFAULT 70
#define HAL_LINK_RATE_DEFAULT 20
#define HAL_LINK_RTT_DEFAULT 50u

void HalLink_Init(void);

void HalLink_SetRssi(int rssi_dbm);
void HalLink_SetConnectionCount(unsigned count);
void HalLink_SetBatteryPercent(int percent);
void HalLink_SetPacketRate(int packets_per_s);
void HalLink_SetLatencyMs(uint32_t latency_ms);

const char *HalLink_GetRemoteName(void);
