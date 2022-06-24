#pragma once

// This file is used to make build time configuration changes for the example application

// Board Selection
// Use this section define which Avnet Starter Kit revision to build for

/#define REV1_BOARD
#define REV2_BOARD

#if defined(REV1_BOARD) AND defined(REV2_BOART)
#error "Both board types defined!  Only define a single board revision"
#endif


