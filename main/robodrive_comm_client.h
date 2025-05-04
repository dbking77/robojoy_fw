#pragma once

#include "robodrive_comm.h"

// Round trip time is typicaly 47 or 67ms
#define ROBODRIVE_COMM_COMMAND_PERIOD_US (50*1000)

void RoboDriveCommClient_Init();

void RoboDriveCommClient_Notify(uint64_t time_us, uint8_t* data, unsigned len);

bool RoboDriveCommClient_Update(uint64_t time_us, bool ready_for_comm, struct robodrive_command* cmd);

uint32_t RoboDriveCommClient_GetAndResetResponseCounter();

uint32_t RoboDriveCommClient_GetLatencyMilliseconds();

// Return battery level in percent, or -1
int RoboDriveCommClient_GetBatteryPercent();