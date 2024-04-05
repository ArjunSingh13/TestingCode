//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   Services.h
///   \since  2016-11-11
///   \brief  Declaration of functions for all services
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// void LevelControlServices_Move(bool on);                                        // returns "status changed"
#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #include "OnOffServices.h"                                                      // Interface to device-specific server functions called by IdentifyServer.c
  #include "NetworkServices.h"                                                    // Interface to device-specific server functions called by IdentifyServer.c
  #include "Zigbee2Dali/Zigbee2Dali.h"                                            // Interface to device-specific server functions called by OnOffServer.c
  #define Services_Init             Zigbee2Dali_Init
  #define Services_Start            Zigbee2Dali_Start
  #define Services_TimeTick         Zigbee2Dali_TimeTick
  #define Services_IsInitializing   Zigbee2Dali_IsInitializing
#elif (defined(SAMSUNG_DEMOKIT) || defined(SILABS_DEMOKIT))
  #include "OnOffServices.h"                                                      // Interface to device-specific server functions called by IdentifyServer.c
  #include "NetworkServices.h"                                                    // Interface to device-specific server functions called by IdentifyServer.c
  #include "Zigbee2DevelopmentBoard/Zigbee2DevelopmentBoard.h"                    // Interface to device-specific server functions called by OnOffServer.c
  #define Services_Init             Zigbee2DevelopmentBoard_Init
  #define Services_Start            Zigbee2DevelopmentBoard_Start
  #define Services_TimeTick()       ;
  #define Services_IsInitializing() false
#endif
