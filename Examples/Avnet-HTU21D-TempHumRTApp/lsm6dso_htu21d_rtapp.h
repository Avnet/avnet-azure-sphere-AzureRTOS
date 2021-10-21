/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */

#pragma once

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum   __attribute__((packed))
{
	IC_ACCEL_TEMPHUM_UNKNOWN, 
    IC_ACCEL_TEMPHUM_HEARTBEAT,
	IC_ACCEL_TEMPHUM_READ_ACCEL_SENSOR, 
    IC_ACCEL_TEMPHUM_READ_TEMP_HUM_SENSOR, 
	IC_ACCEL_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_ACCEL_TEMPHUM_SET_TELEMETRY_SEND_RATE,
    IC_ACCEL_TEMPHUM_SET_ACCEL_SENSOR_SAMPLE_RATE,
    IC_ACCEL_TEMPHUM_SET_TEMPHUM_SENSOR_SAMPLE_RATE
    
} INTER_CORE_CMD_ACCEL_TEMPHUM;

#define RT_TELEMETRY_BUFFER_SIZE 64

// Define the expected data structure. 
typedef struct  __attribute__((packed))
{
    INTER_CORE_CMD_ACCEL_TEMPHUM cmd;
    uint32_t telemtrySendRate;
    uint32_t sensorSampleRateAccel;
    uint32_t sensorSampleRateTempHum;
    float accelX;
    float accelY;
    float accelZ;
    float temp;
    float hum;
    char telemetryJSON[RT_TELEMETRY_BUFFER_SIZE];
} IC_COMMAND_BLOCK_ACCEL_TEMPHUM;

