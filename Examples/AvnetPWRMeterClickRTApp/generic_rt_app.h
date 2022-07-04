/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

#define JSON_STRING_MAX_SIZE 100

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_NEW_CLICK_NAME_UNKNOWN,
	IC_NEW_CLICK_NAME_HEARTBEAT,
	IC_NEW_CLICK_NAME_READ_SENSOR_RESPOND_WITH_TELEMETRY,
	IC_NEW_CLICK_NAME_SET_AUTO_TELEMETRY_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_NEW_CLICK_NAME_READ_SENSOR
} INTER_CORE_CMD_SAMPLE;

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemetrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_NEW_CLICK_NAME_HL_TO_RT;

typedef struct __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemtrySendRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	uint8_t rawData8bit;
	float rawDataFloat;
} IC_COMMAND_BLOCK_NEW_CLICK_NAME_RT_TO_HL;

