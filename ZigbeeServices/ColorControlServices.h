//-------------------------------------------------------------------------------------------------------------------------------------------------------------
///   \file   ColorControlServices.h
///   \since  2018-04-12
///   \brief  Definitions to customize server functions for ColorControl-cluster!
///
/// DISCLAIMER:
///   The content of this file is intellectual property of the Osram GmbH. It is
///   confidential and not intended for public release. All rights reserved.
///   Copyright © Osram GmbH
///
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#if (defined(ZIGBEE2DALI_BB) || defined(ZIGBEE2DALI_V1))
  #include "Zigbee2Dali/Zigbee2Dali.h"                                                        // Interface to device-specific server functions called by OnOffServer.c
  #define ColorControlServices_StartColorTemperatureFadeTime        Zigbee2Dali_StartColorTemperatureFadeTime
  #define ColorControlServices_StartColorTemperatureXYFadeTime      Zigbee2Dali_StartColorFadeTime
  #define ColorControlServices_SetColorTemp                         Zigbee2Dali_SetColorTemp
  #define ColorControlServices_StopColorTemperature                 Zigbee2Dali_StopColorTemp
#elif (defined(SAMSUNG_DEMOKIT) || defined(SILABS_DEMOKIT))
#endif
uint16_t ColorControlServer_GetStartUpColorTemp(uint8_t endpoint);
uint16_t ColorControlServer_GetColorTemp(uint8_t endpoint);
uint16_t ColorControlServer_GetColorTempPhysMin(uint8_t endpoint);
uint16_t ColorControlServer_GetColorTempPhysMax(uint8_t endpoint);
void ColorControlServer_SetColorTempPhysMin(uint8_t endpoint, uint16_t colorTempMired);
void ColorControlServer_SetColorTempMax(uint8_t endpoint, uint16_t colorTempMired);
void ColorControlServer_LevelControlCoupledColorTempChangeCallback(uint8_t endpoint);
