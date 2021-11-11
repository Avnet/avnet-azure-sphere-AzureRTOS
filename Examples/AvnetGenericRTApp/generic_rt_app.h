/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

#include "intercore_generic.h"

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_SAMPLE_UNKNOWN,
	IC_SAMPLE_HEARTBEAT,
	IC_SAMPLE_READ_SENSOR_RESPOND_WITH_TELEMETRY,
	IC_SAMPLE_SET_SAMPLE_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_GENERIC_READ_SENSOR
} INTER_CORE_CMD_SAMPLE;

typedef struct __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t sensorSampleRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	uint8_t rawData8bit;
	float rawDataFloat;
} IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL;


// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemetrySendRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT;