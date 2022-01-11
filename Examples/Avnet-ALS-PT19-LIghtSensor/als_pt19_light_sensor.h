/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#define JSON_STRING_MAX_SIZE 128

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum   __attribute__((packed))
{
    IC_LIGHTSENSOR_UNKNOWN,
    IC_LIGHTSENSOR_HEARTBEAT,
	IC_LIGHTSENSOR_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_LIGHTSENSOR_SAMPLE_RATE,
	/////////////////////////////////////////////////////////////////////////////////
	// Don't change the enums above or the generic RTApp implementation will break //
	/////////////////////////////////////////////////////////////////////////////////
  	IC_LIGHTSENSOR_READ_SENSOR, 

} INTER_CORE_CMD_LIGHTSENSOR;

// Define the expected data structures.  
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LIGHTSENSOR cmd;
    uint32_t sensorSampleRate;
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
} IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT;

typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LIGHTSENSOR cmd;
    uint32_t sensorSampleRate;
    char telemetryJSON[JSON_STRING_MAX_SIZE];  
	////////////////////////////////////////////////////////////////////////////////////////
	// Don't change the declarations above or the generic RTApp implementation will break //
	////////////////////////////////////////////////////////////////////////////////////////
    uint32_t sensorData;
    double lightSensorLuxData;    
} IC_COMMAND_BLOCK_ALS_PT19_RT_TO_HL;
