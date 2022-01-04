/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include "intercore_generic.h"

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_LPS22HH_UNKNOWN, 
    IC_LPS22HH_HEARTBEAT,
	IC_LPS22HH_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_LPS22HH_SET_SAMPLE_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_LPS22HH_READ_SENSOR, 

} INTER_CORE_CMD_LPS22HH;

// Define the data structure that the high level app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LPS22HH cmd;
    uint32_t sensorSampleRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT;

// Define the data structure that the real time app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LPS22HH cmd;
    uint32_t sensorSampleRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    float pressure;
} IC_COMMAND_BLOCK_LPS22HH_RT_TO_HL;
