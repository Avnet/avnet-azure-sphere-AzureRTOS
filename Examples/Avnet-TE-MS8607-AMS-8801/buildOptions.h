#pragma once

// Select the Avnet Starter kit Rev and which click socket contains the Thermo CLICK board

// Board Selection
// Use this section define which Avnet Starter Kit revision to build for
//#define REV1_BOARD    
#define REV2_BOARD      // Note make sure the app_manifest.json file references SPIM ISU0

#if defined(REV1_BOARD) && defined(REV2_BOARD)
#error "Invalid configuration, only one board revision can be defined!"
#endif
