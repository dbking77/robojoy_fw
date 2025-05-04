#include "robodrive_comm_client.h"
#include "gatt_client.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"

#define ROBO_CLIENT_TAG "ROBO_CLIENT"

struct RoboDriveCommClient
{
    struct robodrive_response rsp;
    uint8_t seqnum;
    bool ready_for_comm;
    bool expect_response;
    bool response_valid;
    bool should_disconnect;
    uint64_t last_command_time_us;
    uint32_t latency_ms;

    uint32_t valid_response_counter;
};

struct RoboDriveCommClient g_robojoy_comm;

void RoboDriveCommClient_Init()
{
    g_robojoy_comm.ready_for_comm = false;
    g_robojoy_comm.expect_response = false;
    g_robojoy_comm.response_valid = false;
    g_robojoy_comm.should_disconnect = false;
    g_robojoy_comm.last_command_time_us = 0;
    g_robojoy_comm.latency_ms = 0;
    g_robojoy_comm.seqnum = 0;
    g_robojoy_comm.valid_response_counter = 0;

    struct robodrive_response* rsp = &g_robojoy_comm.rsp;
    memset(rsp, 0, sizeof(struct robodrive_response));
}

void RoboDriveCommClient_Notify(uint64_t time_us, uint8_t* data, unsigned len)
{
    if (!g_robojoy_comm.ready_for_comm || g_robojoy_comm.should_disconnect || !g_robojoy_comm.expect_response) 
    {
        ESP_LOGW(ROBO_CLIENT_TAG, "Got unexpected response : ready_for_comm %d, should_disconnect %d, expect_response %d",
            g_robojoy_comm.ready_for_comm, g_robojoy_comm.should_disconnect, !g_robojoy_comm.expect_response
        );
        return;
    }

    if (len != sizeof(struct robodrive_response))
    {
        ESP_LOGW(ROBO_CLIENT_TAG, "Invalid response size %u", len);
        g_robojoy_comm.should_disconnect = true;
        return;
    }

    memcpy(&g_robojoy_comm.rsp, data, sizeof(struct robodrive_response));

    if (g_robojoy_comm.rsp.seqnum != g_robojoy_comm.seqnum)
    {
        ESP_LOGW(ROBO_CLIENT_TAG, "Invalid response sequence number: got %u expected %u", (unsigned)g_robojoy_comm.rsp.seqnum, (unsigned)g_robojoy_comm.seqnum);
        g_robojoy_comm.should_disconnect = true;
        return;
    } 

    g_robojoy_comm.response_valid = true;
    g_robojoy_comm.expect_response = false;
    uint64_t latency_us = time_us - g_robojoy_comm.last_command_time_us;
    g_robojoy_comm.latency_ms = latency_us/1000;
    ++g_robojoy_comm.valid_response_counter;
    //ESP_LOGI(ROBO_CLIENT_TAG, "Valid response latency ms %u", (unsigned)g_robojoy_comm.latency_ms);
    GattClient_ReadRSSI();
}

bool RoboDriveCommClient_Update(uint64_t time_us, bool ready_for_comm, struct robodrive_command* cmd)
{
    bool send_cmd = false;
    if (!ready_for_comm)
    {
        g_robojoy_comm.expect_response = false;
        g_robojoy_comm.last_command_time_us = 0;
    }
    else if (g_robojoy_comm.expect_response)
    {
        // if response is expected but hasn't arrived, don't send another command
        // connnected communication
    }
    else 
    {
        uint64_t timediff_us = time_us - g_robojoy_comm.last_command_time_us;
        if (timediff_us >= ROBODRIVE_COMM_COMMAND_PERIOD_US)
        {
            g_robojoy_comm.expect_response = true;
            g_robojoy_comm.last_command_time_us = time_us;
            send_cmd = true;
            cmd->seqnum = ++g_robojoy_comm.seqnum;
            cmd->key = g_robojoy_comm.rsp.key;
            cmd->hdr = default_robodrive_hdr;
        }
    }

    g_robojoy_comm.ready_for_comm = ready_for_comm;
    return send_cmd;
} 


uint32_t RoboDriveCommClient_GetAndResetResponseCounter()
{
    uint32_t count = g_robojoy_comm.valid_response_counter;
    g_robojoy_comm.valid_response_counter = 0;
    return count;
}

uint32_t RoboDriveCommClient_GetLatencyMilliseconds()
{
    return g_robojoy_comm.latency_ms;
}

int RoboDriveCommClient_GetBatteryPercent()
{
    return g_robojoy_comm.response_valid ? g_robojoy_comm.rsp.batt_percent : -1;
}