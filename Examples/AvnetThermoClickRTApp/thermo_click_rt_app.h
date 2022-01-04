/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#define JSON_STRING_MAX_SIZE 64

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application.
typedef enum   __attribute__((packed))
{
    IC_THERMO_CLICK_UNKNOWN,
    IC_THERMO_CLICK_HEARTBEAT,
	IC_THERMO_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_THERMO_CLICK_SAMPLE_RATE,
	IC_THERMO_CLICK_READ_SENSOR
} INTER_CORE_CMD_THERMO_CLICK;

// Define the data structure that the high level app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_THERMO_CLICK cmd;
    uint32_t sensorSampleRate;
} IC_COMMAND_BLOCK_THERMO_CLICK_HL_TO_RT;

// Define the data structure that the real time app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_THERMO_CLICK cmd;
    uint32_t sensorSampleRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE+1];
	float temperature;
} IC_COMMAND_BLOCK_THERMO_CLICK_RT_TO_HL;