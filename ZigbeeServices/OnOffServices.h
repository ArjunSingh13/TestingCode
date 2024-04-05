//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   OnOffServices.h
///   \since  2016-11-11
///   \brief  Declaration of functions called by OnOff-cluster of a server. Customizing the interface!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// void OnOffServices_OnOff(bool on);                                           // return void, because an error cannot be forwarded by the calling function
#include "Zigbee2Dali.h"                                                        // Interface to device-specific server functions called by OnOffServer.c
#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #define OnOffServices_OnOff           Zigbee2Dali_OnOff
#elif (defined(SAMSUNG_DEMOKIT) || defined(SILABS_DEMOKIT))
  #define OnOffServices_OnOff           Zigbee2DevelopmentBoard_OnOff
#endif
