/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

#include "intercore_generic.h"

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_GROVE_GPS_UNKNOWN, 
    IC_GROVE_GPS_HEARTBEAT,
	IC_GROVE_GPS_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_GROVE_GPS_SET_SAMPLE_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_GROVE_GPS_READ_SENSOR, 

} INTER_CORE_CMD_GROVE_GPS;

typedef struct __attribute__((packed))
{
	INTER_CORE_CMD_GROVE_GPS cmd;
	uint32_t sensorSampleRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	double lat;
    double lon;
    int fix_qual;
	int numsats;
    float alt;
} IC_COMMAND_BLOCK_GROVE_GPS_RT_TO_HL;


// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
	INTER_CORE_CMD_GROVE_GPS cmd;
	uint32_t telemetrySendRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT;
