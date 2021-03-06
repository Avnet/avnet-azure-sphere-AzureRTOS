/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

#define JSON_STRING_MAX_SIZE 100

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_AIRQUALITY5_UNKNOWN,
	IC_AIRQUALITY5_HEARTBEAT,
	IC_AIRQUALITY5_READ_SENSOR_RESPOND_WITH_TELEMETRY,
	IC_AIRQUALITY5_SET_AUTO_TELEMETRY_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
	IC_AIRQUALITY5_READ_SENSOR
} INTER_CORE_CMD_SAMPLE;

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemetrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT;

typedef struct __attribute__((packed))
{
	INTER_CORE_CMD_SAMPLE cmd;
	uint32_t telemtrySendRate;
	char telemetryJSON[JSON_STRING_MAX_SIZE];
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
	uint16_t no2;
	uint16_t nh3;
	uint16_t co;
} IC_COMMAND_BLOCK_AIRQUALITY5_RT_TO_HL;

