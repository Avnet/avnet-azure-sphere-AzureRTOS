/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#define JSON_STRING_MAX_SIZE 128

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application.
typedef enum   __attribute__((packed))
{
    IC_GENERIC_UNKNOWN,
    IC_GENERIC_HEARTBEAT,
	IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_GENERIC_SAMPLE_RATE

} INTER_CORE_CMD_GENERIC;

// Define the data structure that the high level app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_GENERIC cmd;
    uint32_t sensorSampleRate;
} IC_COMMAND_BLOCK_GENERIC_HL_TO_RT;

// Define the data structure that the real time app sends
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_GENERIC cmd;
    uint32_t sensorSampleRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];
} IC_COMMAND_BLOCK_GENERIC_RT_TO_HL;
