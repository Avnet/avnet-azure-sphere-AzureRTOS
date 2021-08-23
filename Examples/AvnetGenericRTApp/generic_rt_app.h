/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_UNKNOWN,
		IC_HEARTBEAT,
		IC_READ_SENSOR,
		IC_READ_SENSOR_RESPOND_WITH_TELEMETRY,
		IC_SET_SAMPLE_RATE
} INTER_CORE_CMD;

typedef struct __attribute__((packed))
{
	INTER_CORE_CMD cmd;
	uint32_t sensorSampleRate;
	uint8_t rawData8bit;
	float rawDataFloat;
} IC_COMMAND_BLOCK_GENERIC_RT_APP;