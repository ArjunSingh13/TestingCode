//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   LevelControlServices.h
///   \since  2016-11-11
///   \brief  Definitions to customize server functions for LevelControl-cluster!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

// void LevelControlServices_Move(bool on);                                        // returns "status changed"
#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #include "Zigbee2Dali/Zigbee2Dali.h"                                                        // Interface to device-specific server functions called by OnOffServer.c
  #define LevelControlServices_StartFadeTime    Zigbee2Dali_StartFadeTime
  #define LevelControlServices_StartFadeRate    Zigbee2Dali_StartFadeRate
  #define LevelControlServices_StopFade         Zigbee2Dali_StopFade
#elif (defined(SAMSUNG_DEMOKIT) || defined(SILABS_DEMOKIT))
  #define LevelControlServices_StartFadeTime    Zigbee2DevelopmentBoard_StartFadeTime
  #define LevelControlServices_StartFadeRate    Zigbee2DevelopmentBoard_StartFadeRate
  #define LevelControlServices_StopFade         Zigbee2DevelopmentBoard_StopFade
#endif
void LevelControlServer_RefreshCurrentOutputLevel(uint8_t endpoint);
void LevelControlServer_ActualLevelCallback(uint8_t endpoint, uint8_t level);
void LevelControlServer_SetOutput(uint8_t endpoint, uint8_t level, uint16_t fadeTime100ms);
