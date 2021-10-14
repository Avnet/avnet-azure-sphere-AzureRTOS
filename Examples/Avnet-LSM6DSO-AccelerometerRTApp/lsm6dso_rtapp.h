/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum   __attribute__((packed))
{
	IC_LSM6DSO_UNKNOWN, 
    IC_LSM6DSO_HEARTBEAT,
	IC_LSM6DSO_READ_SENSOR, 
	IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_LSM6DSO_SET_TELEMETRY_SEND_RATE,
    IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE,
    
} INTER_CORE_CMD_LSM6DSO;

#define RT_TELEMETRY_BUFFER_SIZE 64

// Define the expected data structure.  Note this struct came from the AvnetGroveGPS real time application code
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_LSM6DSO cmd;
    uint32_t telemtrySendRate;
    uint32_t sensorSampleRate;
    float accelX;
    float accelY;
    float accelZ;
    char telemetryJSON[RT_TELEMETRY_BUFFER_SIZE];
} IC_COMMAND_BLOCK_LSM6DSO;

