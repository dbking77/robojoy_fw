#include "hal_link.h"

#include "gatt_client.h"
#include "robodrive_comm_client.h"

#include <string.h>

static int g_rssi = HAL_LINK_RSSI_DEFAULT;
static unsigned g_connection_count = HAL_LINK_CONN_DEFAULT;
static int g_battery_percent = HAL_LINK_BATT_DEFAULT;
static int g_packets_per_s = HAL_LINK_RATE_DEFAULT;
static uint32_t g_latency_ms = HAL_LINK_RTT_DEFAULT;
static char g_remote_name[32];

void HalLink_Init(void)
{
    g_rssi = HAL_LINK_RSSI_DEFAULT;
    g_connection_count = HAL_LINK_CONN_DEFAULT;
    g_battery_percent = HAL_LINK_BATT_DEFAULT;
    g_packets_per_s = HAL_LINK_RATE_DEFAULT;
    g_latency_ms = HAL_LINK_RTT_DEFAULT;
    g_remote_name[0] = '\0';
}

void HalLink_SetRssi(int rssi_dbm)
{
    g_rssi = rssi_dbm;
}

void HalLink_SetConnectionCount(unsigned count)
{
    g_connection_count = count;
}

void HalLink_SetBatteryPercent(int percent)
{
    g_battery_percent = percent;
}

void HalLink_SetPacketRate(int packets_per_s)
{
    if (packets_per_s < 0) {
        packets_per_s = 0;
    }
    g_packets_per_s = packets_per_s;
}

void HalLink_SetLatencyMs(uint32_t latency_ms)
{
    g_latency_ms = latency_ms;
}

const char *HalLink_GetRemoteName(void)
{
    return g_remote_name;
}

void GattClient_SetRemoteName(const char *name)
{
    if (name == NULL) {
        name = "";
    }
    strncpy(g_remote_name, name, sizeof(g_remote_name) - 1);
    g_remote_name[sizeof(g_remote_name) - 1] = '\0';
}

int GattClient_GetRSSI(void)
{
    return g_rssi;
}

unsigned GattClient_GetConnectionCount(void)
{
    return g_connection_count;
}

int RoboDriveCommClient_GetBatteryPercent(void)
{
    return g_battery_percent;
}

uint32_t RoboDriveCommClient_GetAndResetResponseCounter(void)
{
    /* lvgl_robot_control.c multiplies this by 10 and treats the result as pkt/s. */
    return (uint32_t)(g_packets_per_s / 10);
}

uint32_t RoboDriveCommClient_GetLatencyMilliseconds(void)
{
    return g_latency_ms;
}
