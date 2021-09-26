/* Copyright (c) Avnet Incorporated. All rights reserved.
   Licensed under the MIT License. */
#pragma once

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum __attribute__((packed))
{
	IC_UNKNOWN,
		IC_WD_SET_INTERVAL,
		IC_WD_TICKLE_WATCH_DOG,
		IC_WD_START,
		IC_WD_STOP
} WD_INTER_CORE_CMD;

typedef struct __attribute__((packed))
{
	WD_INTER_CORE_CMD cmd;
	uint32_t watchDogInterval;
} IC_COMMAND_BLOCK_HW_WD;