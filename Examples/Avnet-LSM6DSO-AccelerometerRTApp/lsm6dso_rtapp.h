/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "intercore_generic.h"

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_LSM6DSO_UNKNOWN, 
    IC_LSM6DSO_HEARTBEAT,
	IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_LSM6DSO_SET_SAMPLE_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_LSM6DSO_READ_SENSOR, 
    IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE,

} INTER_CORE_CMD_LSM6DSO;

// Define the data structure that the high level app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LSM6DSO cmd;
    uint32_t telemtrySendRate;
    uint32_t sensorSampleRate;
} IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT;

// Define the data structure that the real time app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LSM6DSO cmd;
    uint32_t telemtrySendRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    uint32_t sensorSampleRate;
    float accelX;
    float accelY;
    float accelZ;
} IC_COMMAND_BLOCK_LSM6DSO_RT_TO_HL;


