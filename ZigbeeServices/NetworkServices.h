//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   NetworkServices.h
///   \since  2016-11-11
///   \brief  Declaration of functions for networking funktions
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// void LevelControlServices_Move(bool on);                                        // returns "status changed"
#include "Zigbee2Dali.h"                                                        // Interface to device-specific server functions called by OnOffServer.c
#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #define Services_TriggerNetworkLeave   Zigbee2Dali_TriggerNetworkLeave
#elif (defined(SAMSUNG_DEMOKIT) || defined(SILABS_DEMOKIT))
  #define Services_TriggerNetworkLeave   Zigbee2DevelopmentBoard_TriggerNetworkLeave
#endif
