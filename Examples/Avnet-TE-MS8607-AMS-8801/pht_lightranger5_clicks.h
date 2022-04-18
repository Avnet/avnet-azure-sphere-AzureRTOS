#pragma once

/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
   
#define JSON_STRING_MAX_SIZE 100

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum
{
	IC_PHT_LIGHTRANGER5_UNKNOWN,
	IC_PHT_LIGHTRANGER5_HEARTBEAT,
	IC_PHT_LIGHTRANGER5_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_PHT_LIGHTRANGER5_SET_AUTO_TELEMETRY_RATE,
	IC_PHT_LIGHTRANGER5_READ_SENSOR
} INTER_CORE_CMD_PHT_CLICK;
typedef uint8_t cmdType;

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
    uint8_t cmd;
    uint32_t telemtrySendRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_PHT_LIGHTRANGER5_HL_TO_RT;

typedef struct  __attribute__((packed))
{
    uint8_t cmd;
    uint32_t telemtrySendRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];  
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    float temp;
	float hum;
	float pressure;
	int range_mm;
} IC_COMMAND_BLOCK_PHT_LIGHTRANGER5_RT_TO_HL;